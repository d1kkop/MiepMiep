#include "PacketHelper.h"
#include "BinSerializer.h"
#include "Common.h"
#include "Endpoint.h"
#include "Socket.h"
#include "PerThreadDataProvider.h"
#include "Util.h"
#include "JobSystem.h"
#include "Network.h"
#include "LinkManager.h"
#include "Link.h"
#include "Session.h"
#include <iostream>


namespace MiepMiep
{
	// --- Packet ------------------------------------------------------------------------------------------

	RecvPacket::RecvPacket(byte id, u32 len, byte flags):
		m_Id(id),
		m_Data(reserveN<byte>(MM_FL, len)),
		m_Length(len),
		m_Flags(flags)
	{
		// Remainder is zeroed by 'calloc'.
	//	cout << "Pack constructor." << endl;
	}

	RecvPacket::RecvPacket(byte id, class BinSerializer& bs):
		RecvPacket(id, bs.data(), bs.length(), 0, true)
	{
	//	cout << "Pack constructor." << endl;
	}

	RecvPacket::RecvPacket(byte id, const byte* data, u32 len, byte flags, bool copy):
		m_Id(id),
		m_Data(copy ? reserveN<byte>(MM_FL, len) : const_cast<byte*>(data)),
		m_Length(len),
		m_Flags(flags)
	{
		if ( copy ) Platform::memCpy(m_Data, len, data, len);
	//	cout << "Pack constructor." << endl;
	}

	RecvPacket::RecvPacket(const RecvPacket& p):
		RecvPacket(p.m_Id, p.m_Data, p.m_Length, p.m_Flags, true)
	{
	//	cout << "Pack const copy constructor." << endl;
	}

	RecvPacket::RecvPacket(RecvPacket&& p) noexcept:
		m_Id(p.m_Id),
		m_Data(p.m_Data),
		m_Length(p.m_Length),
		m_Flags(p.m_Flags)
	{
		p.m_Data = nullptr;
	//	cout << "Pack non-const move constructor." << endl;
	}

	RecvPacket& RecvPacket::operator=(const RecvPacket& p)
	{
		m_Id = p.m_Id;
		m_Length = p.m_Length;
		m_Flags  = p.m_Flags;
		releaseN(m_Data);
		m_Data = reserveN<byte>(MM_FL, m_Length);
		Platform::memCpy(m_Data, m_Length, p.m_Data, m_Length);
	//	cout << "Pack asignment." << endl;
		return *this;
	}

	RecvPacket& RecvPacket::operator=(RecvPacket&& p) noexcept
	{
		Platform::copy<RecvPacket>(this, &p, 1);
		p.m_Data = nullptr;
	//	cout << "Pack non-const move asignment." << endl;
		return *this;
	}

	RecvPacket::~RecvPacket()
	{
		releaseN(m_Data);
	//	cout << "Pack destructor." << endl;
	}


	// --- PacketHelper ------------------------------------------------------------------------------------------

	byte PacketHelper::makeChannelAndFlags( byte channel, bool relay, bool sysBit, bool isFirstFragment, bool isLastFragment )
	{
		assert( channel <= MM_CHANNEL_MASK );
		byte channelAndFlags = channel & MM_CHANNEL_MASK;
		channelAndFlags |= relay? MM_RELAY_BIT : 0;
		channelAndFlags |= sysBit? MM_SYSTEM_BIT : 0;
		channelAndFlags |= isFirstFragment ? MM_FRAGMENT_FIRST_BIT : 0;
		channelAndFlags |= isLastFragment ? MM_FRAGMENT_LAST_BIT : 0;
		return channelAndFlags;
	}

	bool PacketHelper::beginUnfragmented( BinSerializer& bs, u32 seq, byte compType, byte dataId, byte channel, bool relay, bool sysBit )
	{
		bs.reset();
		__CHECKEDB( bs.write( seq ) );
		__CHECKEDB( bs.write( compType ) );
		__CHECKEDB( bs.write( makeChannelAndFlags( channel, relay, sysBit, true, true ) ) );
		if ( dataId != InvalidByte )
		{
			__CHECKEDB( bs.write( dataId ) );
		}
		return true;
	}

	bool PacketHelper::beginUnfragmented( BinSerializer& bs, byte compType, byte dataId, byte channel, bool relay, bool sysBit )
	{
		bs.reset();
		__CHECKEDB( bs.moveWrite(8) ); // reserved for linkId and seq
		__CHECKEDB( bs.write( compType ) );
		__CHECKEDB( bs.write( makeChannelAndFlags( channel, relay, sysBit, true, true ) ) );
		if ( dataId != InvalidByte )
		{
			__CHECKEDB( bs.write( dataId ) );
		}
		return true;
	}

	bool PacketHelper::createNormalPacket(vector<sptr<const NormalSendPacket>>& framgentsOut,  
										  byte compType, byte dataId,
										  const BinSerializer** serializers, u32 numSerializers,
										  byte channel, bool relay, bool sysBit, i32 maxFragmentSize)
	{
		// actual fragmentSize is somewhat lower due to fragment hdr overhead, subtract this, so that fragmentSize is never exceeded
		maxFragmentSize -= MM_MIN_HDR_SIZE;
		// ensure we have at least space to store data
		if ( maxFragmentSize <= MM_MIN_HDR_SIZE*2 )
		{
			throw;
			assert(false);
			return false;
		}
		byte channelAndFlags = makeChannelAndFlags(channel, relay, sysBit, true, false );
		// --
		u32 totalLength = 0;
		for ( u32 i=0; i< numSerializers; ++i )
		{
			totalLength += serializers[i]->length();
		}
		u32 offset = 0;
		bool quit  = false;
		bool isFirstFragment = true;
		u32 srIdx = 0;
		do // allow zero length payload packets
		{
			auto sp = make_shared<NormalSendPacket>();
			BinSerializer& fragment = sp->m_PayLoad;
			u32 writeLen = Util::min(totalLength, (u32)maxFragmentSize);
			totalLength -= writeLen;
			if ( 0 == totalLength )
			{
				channelAndFlags |= MM_FRAGMENT_LAST_BIT;
				quit = true;
			}
			__CHECKEDB( fragment.write( compType ) );
			__CHECKEDB( fragment.write( channelAndFlags ) );
			if ( isFirstFragment )
			{
				__CHECKEDB( fragment.write( dataId ) );
				channelAndFlags &= ~(MM_FRAGMENT_FIRST_BIT);
				isFirstFragment  = false;
			}
			while ( srIdx < numSerializers && writeLen > 0 )
			{
				u32 leftOverInSerializer = serializers[srIdx]->length()-offset;
				if ( writeLen < leftOverInSerializer )
				{
					__CHECKEDB( fragment.write( serializers[srIdx]->data()+offset, writeLen ) );
					offset += writeLen;
					break;
				}
				else
				{
					__CHECKEDB( fragment.write( serializers[srIdx]->data()+offset, leftOverInSerializer ) );
					offset = 0;
					writeLen -= leftOverInSerializer;
					srIdx++;
				}			
			}
			framgentsOut.emplace_back( sp );
		} while (!quit);
		return true; // jeej
	}

	bool PacketHelper::tryReassembleBigPacket(sptr<const RecvPacket>& finalPack, std::map<u32, sptr<const RecvPacket>>& fragments, u32 seq, u32& seqBegin, u32& seqEnd)
	{
		// try find begin
		seqBegin = seq;
		auto itBegin = fragments.find( seqBegin );
		while ( itBegin != fragments.end() )
		{
			const sptr<const RecvPacket>& pack = itBegin->second;
			if ( (pack->m_Flags & MM_FRAGMENT_FIRST_BIT) != 0 )
			{
				// found begin, try find end now..
				seqEnd = seq;
				auto itEnd = fragments.find( seqEnd );
				while ( itEnd != fragments.end() )
				{
					const sptr<const RecvPacket>& pack = itEnd->second;
					if ( (pack->m_Flags & MM_FRAGMENT_LAST_BIT) != 0 )
					{
						// all fragments available, re-assemble framgents to one pack
						finalPack = reAssembleBigPacket(fragments, seqBegin, seqEnd);
						return true;
					}
					itEnd = fragments.find( ++seqEnd );
				}
				return false;
			}
			itBegin = fragments.find( --seqBegin );
		}
		return false;
	}

	sptr<const RecvPacket> PacketHelper::reAssembleBigPacket(std::map<u32, sptr<const RecvPacket>>& fragments, u32 seqBegin, u32 seqEnd)
	{
		// Count total length.
		u32 totalLen = 0;
		u32 curSeq = seqBegin;
		while (curSeq != seqEnd+1) // take into account that seq wraps
		{
			totalLen += fragments[curSeq]->m_Length;
			curSeq++;
		}

		// Allocate data for packet (copy id and flags from first fragment)
		auto& firstFrag = fragments[seqBegin];
		auto finalPack  = make_shared<RecvPacket>( firstFrag->m_Id, totalLen, firstFrag->m_Flags );

		// Copy each fragment
		curSeq = seqBegin;
		u32 curLen = 0;
		while (curSeq != seqEnd+1) // take into count that seq wraps
		{
			auto fragIt = fragments.find(curSeq);
			const sptr<const RecvPacket>& frag = fragIt->second;
			Platform::memCpy( finalPack->m_Data + curLen, (totalLen-curLen), frag->m_Data, frag->m_Length );
			curLen += frag->m_Length;
			fragments.erase( fragIt );
			curSeq++;
		}

		return finalPack;
	}

	bool PacketHelper::isSeqNewer(u32 incoming, u32 having)
	{
		u32 diff = incoming - having;
		return diff <= MM_NEWER_SEQ_RANGE;
	}

}

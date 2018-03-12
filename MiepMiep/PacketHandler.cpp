#pragma once

#include "PacketHandler.h"
#include "BinSerializer.h"
#include "Common.h"
#include "Endpoint.h"
#include "Socket.h"
#include "PerThreadDataProvider.h"
#include "Util.h"
#include "JobSystem.h"
#include <iostream>


namespace MiepMiep
{
	IPacketHandler::IPacketHandler(Network& network):
		ParentNetwork(network)
	{
	}

	void IPacketHandler::handle(const sptr<const class ISocket>& sock)
	{
		auto sThis = ptr<IPacketHandler>();
		m_Network.get<JobSystem>()->addJob( [=]()
		{
			byte buff[MM_MAX_RECVSIZE];
			u32 rawSize = MM_MAX_RECVSIZE; // buff size in
			Endpoint etp;
			i32 err;
			ERecvResult res = sock->recv( buff, rawSize, etp, &err );

			if ( err != 0 )
			{
				LOGW( "Socket recv error: %d.", err );
				return;
			}

			if ( res == ERecvResult::Succes )
			{
				if ( rawSize >= MM_MIN_HDR_SIZE )
				{
					BinSerializer& bs = PerThreadDataProvider::getSerializer( false );
					bs.resetTo( buff, rawSize, rawSize );
					// thi etp is local, go to shared heap object
					sThis->handleSpecial( bs, *etp.getCopyDerived() );
				}
				else
				{
					LOGW( "Received packet with less than %d bytes (= Hdr size), namely %d. Packet discarded.", MM_MIN_HDR_SIZE, rawSize );
				}
			}
		} );
	}


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
		RecvPacket(id, bs.data(), bs.length(), 0)
	{
	//	cout << "Pack constructor." << endl;
	}

	RecvPacket::RecvPacket(byte id, const byte* data, u32 len, byte flags):
		m_Id(id),
		m_Data(reserveN<byte>(MM_FL, len)),
		m_Length(len),
		m_Flags(flags)
	{
		Platform::memCpy(m_Data, len, data, len);
	//	cout << "Pack constructor." << endl;
	}

	RecvPacket::RecvPacket(const RecvPacket& p):
		RecvPacket(p.m_Id, p.m_Data, p.m_Length, p.m_Flags)
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

	bool PacketHelper::createNormalPacket(vector<sptr<const struct NormalSendPacket>>& framgentsOut,  
										  byte compType, byte dataId,
										  const BinSerializer** serializers, u32 numSerializers,
										  byte channel, bool relay, bool sysBit, u32 fragmentSize)
	{
		// actual fragmentSize is somewhat lower due to fragment hdr overhead, subtract this, so that fragmentSize is never exceeded
		fragmentSize -= MM_FRAGMENT_HDR_SIZE;
		assert ( fragmentSize > MM_FRAGMENT_HDR_SIZE*2 );
		byte channelAndFlags = channel & MM_CHANNEL_MASK;
		channelAndFlags |= relay? MM_RELAY_BIT : 0;
		channelAndFlags |= sysBit? MM_SYSTEM_BIT : 0;
		channelAndFlags |= MM_FRAGMENT_FIRST_BIT;
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
			u32 writeLen = Util::min(totalLength, fragmentSize);
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
			framgentsOut.emplace_back( const_pointer_cast<const NormalSendPacket>(sp) );
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

		return const_pointer_cast<const RecvPacket>( finalPack );
	}

	bool PacketHelper::isSeqNewer(u32 incoming, u32 having)
	{
		u32 diff = incoming - having;
		return diff <= MM_NEWER_SEQ_RANGE;
	}

}

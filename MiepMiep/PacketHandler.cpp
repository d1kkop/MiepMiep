#pragma once

#include "PacketHandler.h"
#include "BinSerializer.h"
#include "Common.h"
#include "Endpoint.h"
#include "Socket.h"
#include "PerThreadDataProvider.h"
#include "Util.h"
#include <iostream>


namespace MiepMiep
{
	IPacketHandler::IPacketHandler(Network& network):
		ParentNetwork(network)
	{
	}

	void IPacketHandler::handle(const sptr<const class ISocket>& sock)
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
			// Minimum size is linkId + some protocol (1 byte)
			if ( rawSize >= MM_MIN_HDR_SIZE )
			{
				BinSerializer& bs = PerThreadDataProvider::getSerializer( false );
				bs.resetTo( buff, rawSize, rawSize );
				handleSpecial( bs, etp );
			}
			else
			{
				LOGW( "Received packet with less than %d bytes. Packet discarded.", MM_MIN_HDR_SIZE );
			}
		}
	}


	// --- Packet ------------------------------------------------------------------------------------------

	RecvPacket::RecvPacket(byte id, u32 len):
		m_Id(id),
		m_Length(len)
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
		byte channelAndFlags = channel & MM_CHANNEL_MASK;
		channelAndFlags |= relay? MM_RELAY_BIT : 0;
		channelAndFlags |= sysBit? MM_SYSTEM_BIT : 0;
		channelAndFlags |= MM_FRAGMENT_FIRST_BIT;
		// --
		u32 len = 0;
		for ( u32 i=0; i< numSerializers; ++i )
		{
			len += serializers[i]->length();
		}
		u32 offset = 0;
		bool quit  = false;
		bool isFirstFragment = true;
		do // allow zero length payload packets
		{
			auto sp = make_shared<NormalSendPacket>( dataId, channel );
			BinSerializer& fragment = sp->m_PayLoad;
			u32 writeLen = Util::min(len, fragmentSize);
			len -= writeLen;
			offset += writeLen;
			if ( 0 == len )
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
			__CHECKEDB( fragment.write( (*serializers)->data()+offset, Util::min( writeLen, (*serializers)->length()-offset ) ) );
			if ( offset + writeLen >= (*serializers)->length() )
			{
				serializers++;
			}
			framgentsOut.emplace_back( const_pointer_cast<const NormalSendPacket>(sp) );
		} while (!quit);
		return true; // jeej
	}

	bool PacketHelper::tryReassembleBigPacket(RecvPacket& finalPack, std::map<u32, RecvPacket>& fragments, u32 seq, u32& seqBegin, u32& seqEnd)
	{
		// try find begin
		seqBegin = seq;
		auto itBegin = fragments.find( seqBegin );
		while ( itBegin != fragments.end() )
		{
			RecvPacket& pack = itBegin->second;
			if ( (pack.m_Flags & MM_FRAGMENT_FIRST_BIT) != 0 )
			{
				// found begin, try find end now..
				seqEnd = seq;
				auto itEnd = fragments.find( seqEnd );
				while ( itEnd != fragments.end() )
				{
					if ( (pack.m_Flags & MM_FRAGMENT_LAST_BIT) != 0 )
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

	RecvPacket PacketHelper::reAssembleBigPacket(std::map<u32, RecvPacket>& fragments, u32 seqBegin, u32 seqEnd)
	{
		// Count total length.
		u32 totalLen = 0;
		u32 curSeq = seqBegin;
		while (curSeq != seqEnd+1) // take into account that seq wraps
		{
			totalLen += fragments[curSeq].m_Length;
			curSeq++;
		}

		// Allocate data for packet
		RecvPacket finalPack(fragments[seqBegin].m_Id, totalLen);

		// Copy each fragment
		curSeq = seqBegin;
		u32 curLen = 0;
		while (curSeq != seqEnd+1) // take into count that seq wraps
		{
			auto fragIt = fragments.find(curSeq);
			const RecvPacket& frag = fragIt->second;
			Platform::memCpy( finalPack.m_Data + curLen, (totalLen-curLen), frag.m_Data, frag.m_Length );
			curLen += frag.m_Length;
			fragments.erase( fragIt );
			curSeq++;
		}

		// use move construct
		return finalPack;
	}

	bool PacketHelper::isSeqNewer(u32 incoming, u32 having)
	{
		u32 diff = incoming - having;
		return diff <= MM_NEWER_SEQ_RANGE;
	}

}

#pragma once

#include "PacketHandler.h"
#include "BinSerializer.h"
#include "Common.h"
#include "Endpoint.h"
#include "Socket.h"
#include "PerThreadDataProvider.h"
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

	Packet::Packet(byte id, u32 len):
		m_Id(id),
		m_Length(len)
	{
		// Remainder is zeroed by 'calloc'.
	//	cout << "Pack constructor." << endl;
	}

	Packet::Packet(byte id, class BinSerializer& bs):
		Packet(id, bs.data(), bs.length(), 0)
	{
	//	cout << "Pack constructor." << endl;
	}

	Packet::Packet(byte id, const byte* data, u32 len, byte flags):
		m_Id(id),
		m_Data(reserveN<byte>(MM_FL, len)),
		m_Length(len),
		m_Flags(flags)
	{
		Platform::memCpy(m_Data, len, data, len);
	//	cout << "Pack constructor." << endl;
	}

	Packet::Packet(const Packet& p):
		Packet(p.m_Id, p.m_Data, p.m_Length, p.m_Flags)
	{
	//	cout << "Pack const copy constructor." << endl;
	}

	Packet::Packet(Packet&& p) noexcept:
		m_Id(p.m_Id),
		m_Data(p.m_Data),
		m_Length(p.m_Length),
		m_Flags(p.m_Flags)
	{
		p.m_Data = nullptr;
	//	cout << "Pack non-const move constructor." << endl;
	}

	Packet& Packet::operator=(const Packet& p)
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

	Packet& Packet::operator=(Packet&& p) noexcept
	{
		Platform::copy<Packet>(this, &p, 1);
		p.m_Data = nullptr;
	//	cout << "Pack non-const move asignment." << endl;
		return *this;
	}

	Packet::~Packet()
	{
		releaseN(m_Data);
	//	cout << "Pack destructor." << endl;
	}


	// --- PacketHelper ------------------------------------------------------------------------------------------

	bool PacketHelper::tryReassembleBigPacket(Packet& finalPack, std::map<u32, Packet>& fragments, u32 seq, u32& seqBegin, u32& seqEnd)
	{
		// try find begin
		seqBegin = seq;
		auto itBegin = fragments.find( seqBegin );
		while ( itBegin != fragments.end() )
		{
			Packet& pack = itBegin->second;
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

	Packet PacketHelper::reAssembleBigPacket(std::map<u32, Packet>& fragments, u32 seqBegin, u32 seqEnd)
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
		Packet finalPack(fragments[seqBegin].m_Id, totalLen);

		// Copy each fragment
		curSeq = seqBegin;
		u32 curLen = 0;
		while (curSeq != seqEnd+1) // take into count that seq wraps
		{
			auto fragIt = fragments.find(curSeq);
			const Packet& frag = fragIt->second;
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
		return diff <= (UINT_MAX>>2);
	}

}

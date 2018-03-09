#pragma once

#include "Network.h"


namespace MiepMiep
{
	class IPacketHandler: public virtual ParentNetwork, public ITraceable
	{
	public:
		IPacketHandler(Network& network);

		// All packets are handled here first and then if 'valid' passed to handleSpecial.
		void handle( const sptr<const class ISocket>& sock );

		virtual void handleSpecial( class BinSerializer& bs, const class Endpoint& etp ) = 0;

	};

	struct Packet
	{
	public:
		Packet() = default; // Auto zeroed, due to 'calloc'
		Packet(byte id, u32 len);
		Packet(byte id, const byte* data, u32 len, byte flags);
		Packet(const Packet& p);
		Packet(Packet&& p);
		Packet& operator=(const Packet& p);
		Packet& operator=(Packet&& p);
		~Packet();
	
		byte  m_Id;
		byte* m_Data;
		u32   m_Length;
		byte  m_Flags;
	};

	struct PacketInfo
	{
		u32 m_Sequence;
		byte m_Flags;
	};

	struct PacketHelper
	{
		static bool tryReassembleBigPacket(Packet& finalPack, std::map<u32, Packet>& fragments, u32 seq, u32& seqBegin, u32& seqEnd);
		static Packet reAssembleBigPacket(std::map<u32, Packet>& fragments, u32 seqBegin, u32 seqEnd);
		static bool isSeqNewer( u32 incoming, u32 having );
	};
}

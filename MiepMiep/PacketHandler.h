#pragma once

#include "ParentNetwork.h"
#include "BinSerializer.h"
#include "Common.h"
#include "Memory.h"
#include <vector>


namespace MiepMiep
{
	class BinSerializer;
	class Network;
	class Session;
	class Link;
	class ISocket;
	class SessionBase;
	struct SocketAddrPair;


	class IPacketHandler: public ParentNetwork, public ITraceable
	{
	public:
		IPacketHandler(Network& network);

		MM_TS void recvFromSocket( const ISocket& sock );
		MM_TS void handleInitialAndPassToLink( BinSerializer& bs, const ISocket& sock, const IAddress& addr );
		MM_TS sptr<Link> getOrCreateLink( u32 linkId, const SocketAddrPair& sap );
		MM_TSC virtual SessionBase* getSession() const { return nullptr; }
	};


	struct RecvPacket
	{
	public:
		RecvPacket() = default; // Auto zeroed, due to 'calloc'.
		RecvPacket(byte id, class BinSerializer& bs);
		RecvPacket(byte id, u32 len, byte flags);
		RecvPacket(byte id, const byte* data, u32 len, byte flags, bool copy);
		RecvPacket(const RecvPacket& p);
		RecvPacket(RecvPacket&& p) noexcept;
		RecvPacket& operator=(const RecvPacket& p);
		RecvPacket& operator=(RecvPacket&& p) noexcept;
		~RecvPacket();
	
		byte  m_Id;
		byte* m_Data;
		u32   m_Length;
		byte  m_Flags;
	};

	struct PacketInfo
	{
		u32  m_Sequence;
		byte m_ChannelAndFlags;
	};

	struct NormalSendPacket
	{
		BinSerializer m_PayLoad;
	};

	struct PacketHelper
	{
		static byte makeChannelAndFlags( byte channel, bool relay, bool sysBit, bool isFirstFragment, bool isLastFragment );
		static bool beginUnfragmented( BinSerializer& bs, u32 linkId, u32 seq, byte compType, byte dataId, byte channel, bool relay, bool sysBit );
		static bool beginUnfragmented( BinSerializer& bs, byte compType, byte dataId, byte channel, bool relay, bool sysBit );
		static bool createNormalPacket( vector<sptr<const struct NormalSendPacket>>& framgentsOut, byte compType, byte dataId, 
										const BinSerializer** serializers, u32 numSerializers, byte channel, bool relay, bool sysBit, u32 fragmentSize );
		static bool tryReassembleBigPacket(sptr<const RecvPacket>& finalPack, std::map<u32, sptr<const RecvPacket>>& fragments, u32 seq, u32& seqBegin, u32& seqEnd);
		static sptr<const RecvPacket> reAssembleBigPacket(std::map<u32, sptr<const RecvPacket>>& fragments, u32 seqBegin, u32 seqEnd);
		static bool isSeqNewer( u32 incoming, u32 having );
	};
}

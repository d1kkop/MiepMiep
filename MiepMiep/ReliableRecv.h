#pragma once

#include "Link.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	class ReliableRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableRecv; }

		MM_TS void receive( class BinSerializer& bs, const struct PacketInfo& pi );
		MM_TS void proceedRecvQueue();
		MM_TS void handlePacket( const RecvPacket& pack );
		MM_TS void handleRpc( const RecvPacket& pack );
		
	private:
		mutex m_RecvMutex;
		u32 m_RecvSequence;
		map<u32, pair<RecvPacket, u32>> m_OrderedPackets;
		map<u32, RecvPacket> m_OrderedFragments;
	};
}

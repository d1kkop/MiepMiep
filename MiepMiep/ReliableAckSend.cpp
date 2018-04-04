#include "ReliableAckSend.h"
#include "Link.h"
#include "LinkStats.h"
#include "PerThreadDataProvider.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	ReliableAckSend::ReliableAckSend(Link& link):
		ParentLink(link),
		m_LastResendTS(0)
	{

	}

	MM_TS void ReliableAckSend::addAck( u32 ack )
	{
		scoped_spinlock lk(m_PacketsMutex);
		m_ReceivedPackets.emplace_back( ack );
	}

	MM_TS void ReliableAckSend::resend()
	{
		auto& bs = PerThreadDataProvider::getSerializer(true);
		__CHECKED( PacketHelper::beginUnfragmented( bs, m_Link.id(), 0, (byte)compType(), 0, (byte)idx(), false, true ) );
		bool hasPendingWrites = false;
		u32 mtu = m_Link.getOrAdd<LinkStats>()->mtuAdjusted();
		scoped_spinlock lk( m_PacketsMutex );
		for ( u32 ack : m_ReceivedPackets )
		{
			__CHECKED( bs.write( ack ) );
			hasPendingWrites = true;
			if ( bs.length() >= mtu )
			{
				m_Link.send( bs.data(), bs.length() );
				__CHECKED( PacketHelper::beginUnfragmented( bs, m_Link.id(), 0, (byte)compType(), 0, (byte)idx(), false, true ) );
				hasPendingWrites = false;
				this_thread::sleep_for( chrono::milliseconds( 2 ) ); // TODO remove
			}
		}
		if ( hasPendingWrites )
		{
			m_Link.send( bs.data(), bs.length() );
		}
		m_ReceivedPackets.clear();
	}

	MM_TS void ReliableAckSend::intervalDispatch( u64 time )
	{
		u32 delay = m_Link.getOrAdd<LinkStats>()->ackAggregateTime();
		if ( time - m_LastResendTS > delay )
		{
			m_LastResendTS = time;
		//	network().get<JobSystem>()->addJob( [] ()
			{
				resend();
			}//);
		}
	}

}

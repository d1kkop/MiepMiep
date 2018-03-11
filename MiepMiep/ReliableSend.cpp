#include "ReliableSend.h"
#include "PerThreadDataProvider.h"
#include "PacketHandler.h"
#include "LinkStats.h"


namespace MiepMiep
{

	ReliableSend::ReliableSend(Link& link):
		ParentLink(link),
		m_SendSequence(0),
		m_LastResendTS(0)
	{
	}

	MM_TS void ReliableSend::enqueue(const vector<sptr<const NormalSendPacket>>& rsp, class IDeliveryTrace* trace)
	{
		scoped_lock lk(m_SendQueueMutex);
		for ( auto& p : rsp )
		{
			assert( p->m_PayLoad.length() <= MM_MAX_FRAGMENTSIZE );
			assert( m_SendQueue.count( m_SendSequence ) == 0 );
			m_SendQueue[m_SendSequence++] = p;
		}
	}

	MM_TS void ReliableSend::resend()
	{
		scoped_lock lk(m_SendQueueMutex);
		for ( auto& kvp : m_SendQueue )
		{
			const NormalSendPacket& sendPack = *kvp.second;
			m_Link.send( sendPack );
		}
	}

	void ReliableSend::resendIfLatencyTimePassed(u64 time)
	{
		u32 delay = m_Link.getOrAdd<LinkStats>()->reliableResendDelay();
		if ( time - m_LastResendTS > delay )
		{
			m_LastResendTS = time;
			resend();
		}
	}

	void ReliableSend::dispatchAckQueueIfAggregateTimePassed(u64 time)
	{

	}

	// Placed here because RPC is alwasy reliable ordered send.
	MM_TS ESendCallResult priv_send_rpc(INetwork& nw, const char* rpcName, BinSerializer& payLoad, const IEndpoint* specific, 
										bool exclude, bool buffer, bool relay, bool sysBit, byte channel, IDeliveryTrace* trace)
	{
		// Avoid rewriting the entire payload just for the rpc name in front.
		// Instead, make a seperate serializer for the name and append 2 serializers.
		BinSerializer bsRpcName;
		__CHECKEDSR( bsRpcName.write( string(rpcName) ) );
		const BinSerializer* binSerializers [] = { &bsRpcName, &payLoad };
		return toNetwork(nw).sendReliable( (byte)EPacketType::RPC, binSerializers, 2, specific, exclude, buffer, relay, sysBit, channel, trace );
	}
}
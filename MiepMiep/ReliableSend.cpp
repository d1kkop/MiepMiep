#include "ReliableSend.h"
#include "Network.h"
#include "Link.h"
#include "LinkStats.h"
#include "Util.h"


namespace MiepMiep
{
	ReliableSend::ReliableSend(Link& link):
		ParentLink(link),
		m_SendSequence(0),
		m_LastResendTS(0)
	{
	}

	MM_TS void ReliableSend::enqueue( const sptr<const NormalSendPacket>& rsp, class IDeliveryTrace* trace )
	{
		scoped_lock lk( m_SendQueueMutex );
		assert( rsp->m_PayLoad.length() <= MM_MAX_FRAGMENTSIZE );
		assert( m_SendQueue.count( m_SendSequence ) == 0 );
		m_SendQueue[m_SendSequence++] = rsp;
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

			// The sequence is specific to each link and packet, all other data in the packet is shared by all links.
			// Before send, print seq in front of shared data.
			byte finalData[MM_MAX_SENDSIZE];
			*(u32*)(finalData) = Util::htonl( kvp.first ); // seq
			Platform::memCpy( finalData + 4, MM_MAX_SENDSIZE-4, sendPack.m_PayLoad.data(), sendPack.m_PayLoad.length() ); // payload
			m_Link.send( finalData, sendPack.m_PayLoad.length()+4 );

		//	this_thread::sleep_for( chrono::milliseconds(2) ); // TODO remove
		}
	}

	MM_TS void ReliableSend::ackList( const vector<u32>& acks )
	{
		scoped_lock lk( m_SendQueueMutex );
		for ( auto seq : acks )
		{
			if ( 0 != m_SendQueue.erase( seq ) )
			{
			//	LOG( "Packet with seq %d in link %s got acked and removed.", seq, m_Link.info() );
			}
		}
		//cout << "Link queue size: " << m_SendQueue.size() << endl;
	}

	MM_TS void ReliableSend::intervalDispatch( u64 time )
	{
		u32 delay = m_Link.getOrAdd<LinkStats>()->reliableResendDelay();
		if ( time - m_LastResendTS > delay )
		{
			m_LastResendTS = time;
			resend();
		}
	}

	// Placed here because RPC is always reliable ordered send.
	MM_TS ESendCallResult priv_send_rpc(INetwork& nw, const char* rpcName, BinSerializer& payLoad, const ISession* session, ILink* exclOrSpecific, 
										bool buffer, bool relay, bool sysBit, byte channel, IDeliveryTrace* trace)
	{
		assert( !buffer || (session) ); // Can only buffer to a session.
		// Avoid rewriting the entire payload just for the rpc name in front.
		// Instead, make a seperate serializer for the name and append 2 serializers.
		BinSerializer bsRpcName;
		__CHECKEDSR( bsRpcName.write( string(rpcName) ) );
		const BinSerializer* binSerializers [] = { &bsRpcName, &payLoad };
		return toNetwork( nw ).sendReliable( (byte)EPacketType::RPC, session, sc<Link*>( exclOrSpecific ), binSerializers, 2, buffer, relay, sysBit, channel, trace );
	}
}
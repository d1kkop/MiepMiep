#include "ReliableRecv.h"
#include "Link.h"
#include "Network.h"
#include "PacketHandler.h"
#include "JobSystem.h"
#include "PerThreadDataProvider.h"
#include "NetworkListeners.h"
#include "Platform.h"
#include "MiepMiep.h"


namespace MiepMiep
{
	// ------ Event --------------------------------------------------------------------------------

	using RpcFunc = void (*)( INetwork&, const IEndpoint&, BinSerializer& );

	struct EventRpc : EventBase
	{
		EventRpc(const IEndpoint& remote, const RpcFunc& rpcFunc, const RecvPacket& pack):
			EventBase(remote),
			m_RpcFunc(rpcFunc),
			m_Pack(pack)
		{
		}

		void process() override
		{
			auto& bs = PerThreadDataProvider::getSerializer(false);
			bs.resetTo( m_Pack.m_Data, m_Pack.m_Length, m_Pack.m_Length );	
			m_NetworkListener->processEvents<IConnectionListener>( [&] (IConnectionListener* l) 
			{
				bs.setRead( 0 );
				m_RpcFunc( *m_Network, *m_Endpoint, bs );
			});
		}

		RpcFunc m_RpcFunc;
		RecvPacket  m_Pack;
	};


	// ------ ReliableRecv --------------------------------------------------------------------------------

	ReliableRecv::ReliableRecv(Link& link):
		ParentLink(link),
		m_RecvSequence(0)
	{
	}

	MM_TS void ReliableRecv::receive(BinSerializer& bs, const PacketInfo& pi)
	{
		scoped_lock lk(m_RecvMutex);

		// Early out if out of date.
		if ( !PacketHelper::isSeqNewer( pi.m_Sequence, m_RecvSequence ) )
		{
			return;
		}

		LOG( "Received reliable seq: %d", pi.m_Sequence );

		// Identifies datatype
		byte packId;
		__CHECKED( bs.read(packId) );

		if ( (pi.m_ChannelAndFlags & MM_FRAGMENT_FIRST_BIT)!=0 && (pi.m_ChannelAndFlags & MM_FRAGMENT_LAST_BIT)!=0 ) // not fragmented
		{
			// Packet may arrive multiple time as recv_sequence is only incremented when expected sequence is received.
			m_OrderedPackets.try_emplace( pi.m_Sequence, /* key */
					make_shared<RecvPacket>( packId, bs.data()+bs.getRead(), bs.getWrite()-bs.getRead(), pi.m_ChannelAndFlags), 1 /* value */
			);
		}
		else // fragmented
		{
			// Fragment may arrive multiple time as recv_sequence is only incremented when expected sequence is received.
			pair<decltype(m_OrderedFragments.begin()),bool> inserted = 
				m_OrderedFragments.try_emplace( pi.m_Sequence, /* key */
					make_shared<RecvPacket>( packId, bs.data()+bs.getRead(), bs.getWrite()-bs.getRead(), pi.m_ChannelAndFlags ) /* value */ );

			if ( !inserted.second )
				return; // Already exists, nothing to do.
			
			// This fragment may have completed the puzzle. Check if all fragments are available to re-assmble big packet.
			sptr<const RecvPacket> finalPack;
			u32 seqBegin, seqEnd;
			if (PacketHelper::tryReassembleBigPacket( finalPack, m_OrderedFragments, pi.m_Sequence, seqBegin, seqEnd ))
			{
				m_OrderedPackets.try_emplace( seqBegin, /* key */
											  finalPack, 1+(seqEnd-seqBegin) /* value */
				);
			}
		}

		// Processed ordered queue as much as possible
		sptr<ReliableRecv> sThis = ptr<ReliableRecv>();
		m_Link.getInNetwork<JobSystem>()->addJob( [sThis]()
		{
			sThis->proceedRecvQueue();
		});
	}

	MM_TS void ReliableRecv::proceedRecvQueue()
	{
		sptr<ReliableRecv> sThis = ptr<ReliableRecv>();
		auto& queue = m_OrderedPackets;
		
		scoped_lock lk(m_RecvMutex);
		auto packIt = queue.find(m_RecvSequence);
		while ( packIt != queue.end() )
		{
			sptr<const RecvPacket> pack = packIt->second.first;
			u32 numFragments = packIt->second.second;
			queue.erase( packIt );

			m_Link.getInNetwork<JobSystem>()->addJob( [sThis, pack]()
			{
				sThis->handlePacket( *pack );
			});
			
			m_RecvSequence += numFragments;
			// try find next ordered sequence
			packIt = queue.find(m_RecvSequence);
		}
	}

	MM_TS void ReliableRecv::handlePacket(const RecvPacket& pack)
	{
		EPacketType pt = sc<EPacketType>( pack.m_Id );
		switch (pt)
		{
		case EPacketType::RPC:
			handleRpc( pack );
			break;

		case EPacketType::UserOffsetStart:
			// TODO add event with user data
			break;

		default:
			LOGW( "Unhandled packet received, data id was %d.", (byte)pt );
			break;
		}
	}

	MM_TS void ReliableRecv::handleRpc(const RecvPacket& pack)
	{
		auto& bs = PerThreadDataProvider::getSerializer( false );
		bs.resetTo( pack.m_Data, pack.m_Length, pack.m_Length );

		// TODO optimize this by setting a flag in the packet if the rpc has been resolved to a static ID
		string rpcName;
		__CHECKED( bs.read( rpcName ) );
		void* rpcAddress = priv_get_rpc_func(rpcName);
		if ( !rpcAddress )
		{
			LOGC( "Cannot find rpc named: %s.", rpcName.c_str() );
			return;
		}
		
		auto rpcFunc = rc<RpcFunc>( rpcAddress );
		bool isSystemCall = pack.m_Flags & MM_SYSTEM_BIT;
		if ( isSystemCall || m_Link.m_Network.allowAsyncCallbacks() ) // call async always
		{
			rpcFunc ( m_Link.m_Network, m_Link.remoteEtp(), bs );
		}
		else
		{
			m_Link.pushEvent<EventRpc>( rpcFunc, pack );
		}
	}

}
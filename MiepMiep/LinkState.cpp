#include "LinkState.h"
#include "Network.h"
#include "Link.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "Rpc.h"
#include "Listener.h"
#include "NetworkEvents.h"
#include "PerThreadDataProvider.h"
#include "MiepMiep.h"
#include "MasterSession.h"
#include "Endpoint.h"


namespace MiepMiep
{
	// ------ Event --------------------------------------------------------------------------------

	struct EventNewConnection : IEvent
	{
		EventNewConnection( const sptr<Link>& link, const MetaData& md, const sptr<const IAddress>& newAddr ):
			IEvent(link, false),
			m_MetaData( md ),
			m_NewAddr(newAddr) { }

		void process() override
		{
			m_Link->getSession()->forListeners( [&] ( ISessionListener* l )
			{
				l->onConnect( m_Link->session(), m_MetaData, *m_NewAddr );
			});
		}

		MetaData m_MetaData;
		sptr<const IAddress> m_NewAddr;
	};

	struct EventDisconnect : IEvent
	{
		EventDisconnect( const sptr<Link>& link, EDisconnectReason reason, const sptr<const IAddress>& discAddr):
			IEvent(link, false),
			m_DiscAddr(discAddr),
			m_Reason(reason) { }

		void process() override
		{
			m_Link->getSession()->forListeners( [&] ( ISessionListener* l )
			{
				l->onDisconnect( m_Link->session(), *m_DiscAddr, m_Reason );
			} );
		}

		sptr<const IAddress> m_DiscAddr;
		EDisconnectReason m_Reason;
	};


	// ------ Rpc --------------------------------------------------------------------------------

	/* New connection and disconnect. */

	// Executes on client.
	// [ endpoint : newEtp ]
	MM_RPC( linkStateNewConnection, MetaData, sptr<IAddress> )
	{
		RPC_BEGIN();
		l.pushEvent<EventNewConnection>( get<0>(tp), get<1>(tp) );
		LOG( "New incoming connection to %s.", l.info() );
	}

	// Executes on client.
	// [ isKick, remoteEtp (if not direct) ]
	MM_RPC( linkStateDisconnect, bool, sptr<IAddress> )
	{
		RPC_BEGIN();
		bool isKick = get<0>( tp );
		sptr<IAddress> remote = get<1>( tp );
		if ( sc<Endpoint&>(*remote).isZero() )
		{
			// is direct
			remote.reset();
		}
		EDisconnectReason reason = isKick?EDisconnectReason::Kicked:EDisconnectReason::Closed;
		if ( remote )
		{ 
			// someone else through client-serv disconnected, not this direct link!
			l.pushEvent<EventDisconnect>( reason, remote );
			LOG( "Remote %s disconnected, reason %d.", remote->toIpAndPort(), (u32)reason );
		}
		else
		{
			// 'disconnect' also pushes EventDisconnect implicitely and removes link afterwards.
			if ( l.disconnect( isKick, Is_Receive, Remove_Link ) )
			{
				// Relay this event if not p2p and is host.
				if ( !s.msd().m_IsP2p && s.imBoss() )
				{
					nw.callRpc2<linkStateDisconnect, bool, sptr<IAddress>>( isKick, l.destination().getCopy(), &l.normalSession(), &l /* <-- excl link */,
																			No_Local, No_Buffer, No_Relay, No_SysBit, MM_RPC_CHANNEL, No_Trace );
				}
			}
		}
	}

	/* Connection results */

	// Executed on client.
	MM_RPC( linkStateAccepted, MetaData, sptr<IAddressList> )
	{
		RPC_BEGIN();
		auto lState = l.get<LinkState>();
		if ( lState )
		{
			if ( lState->accept() )
			{
				const auto& md = get<0>( tp );
				const auto& addrList = get<1>( tp );
				l.pushEvent<EventNewConnection>( md, l.destination().to_ptr() );
				//for ( auto& adr : addrList ) // For client-serv architecture, the serv sends all remote address immediately.
				//{
				//	l.pushEvent<EventNewConnection>( md, l.destination().to_ptr() );
				//}
				LOG( "Link to %s accepted from connect request.", l.info() );
			}
		}
		else
		{
			LOGW( "Received connect accept while there was no linkState created on link %s.", l.info() );
		}
	}

	/* Incoming connect attempt */

	// Executed on client.
	MM_RPC( linkStateConnect, MetaData )
	{
		RPC_BEGIN();

		// For now, make signing 'accept' on a connection and sending the accept list, exclusive with a lock.
		// TODO: this should not be necessary.
		rscoped_lock lck( l.normalSession().dataMutex() );
		if ( l.getOrAdd<LinkState>()->canReceiveConnect() && l.getOrAdd<LinkState>()->accept() )
		{
			auto& md = get<0>( tp );
			l.pushEvent<EventNewConnection>( md, l.destination().to_ptr() );

			if ( !s.msd().m_IsP2p && s.imBoss() ) 
			{
				// NOTE: Because there is no exclusive lock on accepting and sending the addrList, it is possible that
				// we send an address for which we will also receive a newConnection event. This is intended behaviour and handled gracefully.
				sptr<IAddressList> addrList = AddressList::create();
				l.normalSession().forLink( &l, [&] ( Link& lb )
				{
					if ( auto ls = lb.get<LinkState>() )
					{
						if ( ls->state() == ELinkState::Connected )
						{
							addrList->addAddress( lb.destination() );
						}
					}
				} );

				// Addrlist to recipient directly.
				l.callRpc<linkStateAccepted, MetaData, sptr<IAddressList>>(l.normalSession().metaData(), addrList ); 

				// Relay this event if not p2p and is host
				nw.callRpc2<linkStateNewConnection, MetaData, sptr<IAddress>>( md, l.destination().getCopy(), &l.normalSession(), &l /* <-- excl link */,
																			   No_Local, No_Buffer, No_Relay, No_SysBit, MM_RPC_CHANNEL, No_Trace );
			}
		}
		else
		{
			LOGW( "Received connect from link %s while it already had received a connect request.", l.info() );
		}
	}


	// ------ LinkState --------------------------------------------------------------------------------

	LinkState::LinkState(Link& link):
		ParentLink(link),
		m_State(ELinkState::Unknown),
		m_WasAccepted(false),
		m_RemoteConnectReceived(false)
	{
	}

	MM_TS bool LinkState::connect( const MetaData& md )
	{
		// Check state
		{
			scoped_spinlock lk(m_StateMutex);
			if ( m_State != ELinkState::Unknown )
			{
				LOGW( "Can only connect if state is set to 'Unknown." );
				return false;
			}
			m_State = ELinkState::Connecting;
		}
		return m_Link.callRpc<linkStateConnect, MetaData>( md, false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
	}

	MM_TS bool LinkState::accept()
	{
		// Check state
		{
			scoped_spinlock lk( m_StateMutex );
			// State is unknown when new link is added on server and accepted directly from a connect request.
			if ( !m_WasAccepted && !(m_State == ELinkState::Unknown || m_State == ELinkState::Connecting) ) 
			{
				LOGW( "Can only accept from request if state is set to 'Connecting' or 'Unknown'. State change discarded." );
				return false;
			}
			m_State = ELinkState::Connected;
			m_WasAccepted = true;
		}

		return true;
	}

	MM_TS bool LinkState::canReceiveConnect()
	{
		// Check state
		{
			scoped_spinlock lk( m_StateMutex );
			if ( m_State == ELinkState::Disconnected )
			{
				LOGW( "Will not accept connect request when already disconnected." );
				return false;
			}
			if ( m_RemoteConnectReceived )
			{
				LOGW( "Already received remote connect request. Request discarded." );
				return false;
			}
			m_RemoteConnectReceived = true;
		}
		return true;
	}

	MM_TS bool LinkState::disconnect(bool isKick, bool isReceive, bool removeLink)
	{
		bool disconnected = false;

		// Check state
		{
			scoped_spinlock lk( m_StateMutex );
			if ( m_State == ELinkState::Connected )
			{
				disconnected = true;
			}
			m_State = ELinkState::Disconnected;
		}

		// Did state actually change
		if ( disconnected )
		{
			auto reason = isKick ? EDisconnectReason::Kicked : EDisconnectReason::Closed;
			if ( !isReceive )
			{
				m_Link.callRpc<linkStateDisconnect, bool, sptr<IAddress>>( isKick, IAddress::createEmpty(), No_Local, No_Relay, MM_RPC_CHANNEL, nullptr );
			}
			else
			{
				m_Link.pushEvent<EventDisconnect>( reason, m_Link.destination().to_ptr() );
			}
			
		}

		if ( removeLink )
		{
			assert( m_Link.getSession() );
			if ( auto* s = m_Link.getSession() )
			{
				s->removeLink( m_Link );
			}
		}
		
		return disconnected;
	}

	MM_TS ELinkState LinkState::state() const
	{
		scoped_spinlock lk(m_StateMutex);
		return m_State;
	}

}
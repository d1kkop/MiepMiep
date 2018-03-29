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
#include "MasterServer.h"
#include "Endpoint.h"


namespace MiepMiep
{
	// ------ Event --------------------------------------------------------------------------------

	struct EventNewConnection : IEvent
	{
		EventNewConnection( const sptr<Link>& link, const sptr<const IAddress>& newAddr ):
			IEvent(link, false),
			m_NewAddr(newAddr) { }

		void process() override
		{
			m_Link->ses().forListeners( [&] ( ISessionListener* l )
			{
				l->onConnect( m_Link->session(), *m_NewAddr );
			});
		}

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
			m_Link->ses().forListeners( [&] ( ISessionListener* l )
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
	MM_RPC( linkStateNewConnection, sptr<IAddress> )
	{
		RPC_BEGIN();
		l.pushEvent<EventNewConnection>( get<0>(tp) );
		LOG( "New incoming connection to %s.", l.info() );
	}

	// Executes on client.
	// [ endpoint : discEtp,  bool : wasKicked ]
	MM_RPC( linkStateDisconnect, sptr<IAddress>, bool )
	{
		RPC_BEGIN();
		const sptr<IAddress>& remote = get<0>(tp);
		bool isKick = get<1>(tp);
		if ( remote )
		{ // someone else through client-serv disconnected, not this direct link!
			EDisconnectReason reason = isKick?EDisconnectReason::Kicked:EDisconnectReason::Closed;
			l.pushEvent<EventDisconnect>( reason, remote );
			LOG( "Link to %s disconnected, reason %d.", l.info(), (u32)reason );
		}
		else
		{ // direct link disconnected
			bool didDisconnect = l.disconnect( isKick, false );

			// Relay this event if not p2p and is host and is not set up using with a matchmaking server.
			if ( !s.msd().m_IsP2p && !s.msd().m_UsedMatchmaker && s.imBoss() &&
				 didDisconnect ) /* Only send disconnect if did actually disconnect. */
			{
				l.pushEvent<EventNewConnection>( l.destination().to_ptr() );
				nw.callRpc2<linkStateNewConnection, sptr<IAddress>>( l.destination().getCopy(), &s, &l /* <-- excl link */,
																	 No_Local, No_Buffer, No_Relay, No_SysBit, MM_RPC_CHANNEL, No_Trace );
			}

			s.removeLink( l );
		}
	}

	/* Connection results */

	// Executed on client.
	MM_RPC( linkStateAccepted )
	{
		RPC_BEGIN();
		auto lState = l.get<LinkState>();
		if ( lState )
		{
			if ( lState->accept() )
			{
				l.pushEvent<EventNewConnection>( l.destination().to_ptr() );
				LOG( "Link to %s accepted from connect request.", l.info() );
				return;
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

		// Is already part of this session.
		if ( s.hasLink( l ) )
		{
			LOGW( "Link %s tried to connect that is already part of the session.", l.info() );
			return;
		}

		// No longer check pw and max clients as that is done on master server. For local area networks, password and max clients has no use.
		if ( !l.getOrAdd<LinkState>()->canReceiveConnect() )
		{
			l.callRpc<linkStateAccepted>(); // To recipient directly

			// Relay this event if not p2p and is host and is not set up using with a matchmaking server.
			if ( !s.msd().m_IsP2p && !s.msd().m_UsedMatchmaker && s.imBoss() &&
				 l.getOrAdd<LinkState>()->accept() ) /* Only relay event if did actually change state to connected. */
			{
				l.pushEvent<EventNewConnection>( l.destination().to_ptr() );
				nw.callRpc2<linkStateNewConnection, sptr<IAddress>>( l.destination().getCopy(), &s, &l /* <-- excl link */,
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
			if ( !(m_State == ELinkState::Unknown || m_State == ELinkState::Connecting) ) 
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

	MM_TS bool LinkState::disconnect(bool isKick, bool sendMsg)
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

		if ( disconnected )
		{
			auto reason = isKick ? EDisconnectReason::Kicked : EDisconnectReason::Closed;
			m_Link.pushEvent<EventDisconnect>( reason, m_Link.destination().to_ptr() );
			if ( sendMsg )
			{
				m_Link.callRpc<linkStateDisconnect, sptr<IAddress>, bool>( sptr<IAddress>(), isKick, No_Local, No_Relay, MM_RPC_CHANNEL, nullptr );
			}
			return true;
		}
		
		return false; // was not connected before
	}

	MM_TS ELinkState LinkState::state() const
	{
		scoped_spinlock lk(m_StateMutex);
		return m_State;
	}

}
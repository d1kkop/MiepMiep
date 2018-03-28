#include "LinkState.h"
#include "Network.h"
#include "Link.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "Rpc.h"
#include "Listener.h"
#include "NetworkListeners.h"
#include "PerThreadDataProvider.h"
#include "MiepMiep.h"
#include "MasterServer.h"
#include "Endpoint.h"


namespace MiepMiep
{
	// ------ Event --------------------------------------------------------------------------------

	struct EventConnectResult : EventBase
	{
		EventConnectResult(const Link& link, EConnectResult res):
			EventBase(link, false),
			m_Result(res) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] ( IConnectionListener* l )
			{
				l->onConnectResult( *m_Link, m_Result );
			} );
		}

		EConnectResult m_Result;
	};

	struct EventNewConnection : EventBase
	{
		EventNewConnection(const Link& link, const IAddress& newAddr):
			EventBase(link, false),
			m_NewAddr(newAddr.to_ptr()) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onNewConnection( *m_Link, *m_NewAddr );
			});
		}

		sptr<const IAddress> m_NewAddr;
	};

	struct EventDisconnect : EventBase
	{
		EventDisconnect(const Link& link, EDisconnectReason reason, const IAddress& discAddr):
			EventBase(link, false),
			m_DiscAddr(discAddr.to_ptr()),
			m_Reason(reason) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onDisconnect( *m_Link, m_Reason, *m_DiscAddr );
			});
		}

		sptr<const IAddress> m_DiscAddr;
		EDisconnectReason m_Reason;
	};


	// ------ Rpc --------------------------------------------------------------------------------

	/* New connection and disconnect. */

	// [ endpoint : newEtp ]
	MM_RPC( linkStateNewConnection, sptr<IAddress> )
	{
		RPC_BEGIN();
		l.pushEvent<EventNewConnection>( *get<0>(tp) );
		LOG( "New incoming connection to %s.", l.info() );
	}

	// [ endpoint : discEtp,  bool : wasKicked ]
	MM_RPC( linkStateDisconnect, sptr<IAddress>, bool )
	{
		RPC_BEGIN();
		EDisconnectReason reason = get<1>(tp) ? EDisconnectReason::Kicked : EDisconnectReason::Closed;
		l.pushEvent<EventDisconnect>( reason, *get<0>(tp) );
		LOG( "Link to %s disconnected, reason %d.", l.info(), (u32)reason );
	}

	/* Connection results */

	MM_RPC( linkStateAccepted )
	{
		RPC_BEGIN();
		auto lState = l.get<LinkState>();
		if ( lState )
		{
			if ( lState->acceptFromReturnTrip() )
			{
				l.pushEvent<EventConnectResult>( EConnectResult::Fine );
				LOG( "Link to %s accepted from connect request.", l.info() );
				return;
			}
		}
		l.pushEvent<EventConnectResult>( EConnectResult::NotConnecting );
	}

	/* Incoming connect atempt */

	// [meta]
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
		l.callRpc<linkStateAccepted>(); // To recipient directly

		// Relay this event if not p2p and is host.
		if ( !s.msd().m_IsP2p && s.imBoss() )
		{
			nw.callRpc2<linkStateNewConnection, sptr<IAddress>>( l.destination().getCopy(), &s, &l /* <-- excl */,
																 No_Local, No_Buffer, No_Relay, No_SysBit, MM_RPC_CHANNEL, No_Trace  );
		}
	}

	// ------ LinkState --------------------------------------------------------------------------------

	LinkState::LinkState(Link& link):
		ParentLink(link),
		m_State(ELinkState::Unknown),
		m_WasAccepted(false)
	{
	}

	MM_TS bool LinkState::connect( const MetaData& md )
	{
		// Check state
		{
			scoped_spinlock lk(m_StateMutex);

			if ( m_State == ELinkState::Connected )
			{
				LOG( "Connect returned immediately succesful as it was already accepted." );
				return true;
			}

			if ( m_State != ELinkState::Unknown )
			{
				LOGW( "Can only connect if state is set to 'Unknown." );
				return false;
			}
			m_State = ELinkState::Connecting;
		}
		return m_Link.callRpc<linkStateConnect, MetaData>( md, false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
	}

	MM_TS bool LinkState::acceptFromSingleTrip()
	{
		// Check state
		{
			scoped_spinlock lk( m_StateMutex );
			if ( !(m_State == ELinkState::Unknown) )
			{
				LOGW( "Can only accept direct if state is set to 'Unknown'. State change discarded." );
				return false;
			}
			m_State = ELinkState::Connected;
			m_WasAccepted = true;
		}

		// Any other state (also if already connected, return false).
		return true;
	}

	MM_TS bool LinkState::acceptFromReturnTrip()
	{
		// Check state
		{
			scoped_spinlock lk( m_StateMutex );
			if ( !(m_State == ELinkState::Connecting) )
			{
				LOGW( "Can only accept from request if state is set to 'Connecting'. State change discarded." );
				return false;
			}
			m_State = ELinkState::Connected;
			m_WasAccepted = true;
		}

		// Any other state (also if already connected, return false).
		return true;
	}

	MM_TS ELinkState LinkState::state() const
	{
		scoped_spinlock lk(m_StateMutex);
		return m_State;
	}

}
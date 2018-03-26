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
#include "Endpoint.h"
#include "Socket.h"


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
			auto ls = m_Link->get<LinkState>();
			if ( ls )
			{
				ls->getConnectCb()(m_Result);
			}
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

	MM_RPC( linkStateAlreadyConnected )
	{
		RPC_BEGIN();
		l.pushEvent<EventConnectResult>( EConnectResult::AlreadyConnected );
		LOG( "Link to %s ignored, already connected.", l.info() );
	}

	MM_RPC( linkStatePasswordFail )
	{
		RPC_BEGIN();
		l.pushEvent<EventConnectResult>( EConnectResult::InvalidPassword );
		LOG( "Link to %s ignored, invalid password.", l.info() );
	}

	MM_RPC( linkStateMaxClientsReached )
	{
		RPC_BEGIN();
		l.pushEvent<EventConnectResult>( EConnectResult::MaxConnectionsReached );
		LOG( "Link to %s ignored, max connections reached.", l.info() );
	}

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

	// [Pw, meta]
	MM_RPC( linkStateConnect, string, MetaData )
	{
		RPC_BEGIN();

		// Link has no originator (read: listener) if link is set up through master server.
		// In that case, the password and max clients check is done on the master server.
		// Furthermore, links in that case do not have a listener, but are linked to eachother through the master server
		// which uses the links implicitly bound ports.
		// This is different then when using a listener, which uses a chosen port instead.
	//	auto originator = l.originator();
		if ( true )
		{
			// If has originator, that is, it was created from a listener.
			// The link should have no linkState (yet).
			if ( l.has<LinkState>() )
			{
				l.callRpc<linkStateAlreadyConnected>();
				return;
			}

			// TODO replace for get session
			//// Password check
			//const string& pw = get<0>( tp );
			//if ( originator->getPassword() != pw )
			//{
			//	l.callRpc<linkStatePasswordFail>();
			//	return;
			//}

			//// Max clients check
			//if ( originator->getNumClients() >= originator->getMaxClients() )
			//{
			//	l.callRpc<linkStateMaxClientsReached>();
			//	return;
			//}

			bool wasAccepted = l.getOrAdd<LinkState>()->acceptFromSingleTrip();
			if ( wasAccepted )
			{
				LOG( "Link to %s accepted directly.", l.info() );
				l.callRpc<linkStateAccepted>(); // To recipient directly
				nw.callRpc2<linkStateNewConnection, sptr<IAddress>>( l.destination().getCopy(), l.socket(), false, &l.destination(), true /*excl*/ ); // To all others except recipient
				l.pushEvent<EventNewConnection>( l.destination() );
			}
			else
			{
				l.callRpc<linkStateAlreadyConnected>();
			}
		}
		else /* The link was not created from a listener but from a (other) link on an implictly (not chosen) bound port.
				In this case, it is possible that the linkState already exists as two endpoints may simultaneously connect to each other. */
		{
			// For a connect request, do nothing. Since both parties connect, only accept the connection from a round trip instead of a single trip.
			l.callRpc<linkStateAccepted>(); // Send back accepted but do NOT accept the link on this side from a single trip.
		}
	}

	// ------ LinkState --------------------------------------------------------------------------------

	LinkState::LinkState(Link& link):
		ParentLink(link),
		m_State(ELinkState::Unknown),
		m_WasAccepted(false)
	{
	}

	MM_TS bool LinkState::connect( const string& pw, const MetaData& md, const function<void( EConnectResult )>& connectCb )
	{
		// Check state
		{
			scoped_spinlock lk(m_StateMutex);
			m_ConnectCb = connectCb;

			if ( m_WasAccepted )
			{
				LOG( "Connect returned immediately succesful as it was already accepted." );
				return true;
			}

			if ( m_State != ELinkState::Unknown )
			{
				LOGW( "Can only connect if state is set to 'Unknown." );
				assert( false );
				return false;
			}
			m_State = ELinkState::Connecting;
		}
		return m_Link.callRpc<linkStateConnect, string, MetaData>( pw, md, false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
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
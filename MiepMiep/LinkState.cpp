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


namespace MiepMiep
{
	// ------ Event --------------------------------------------------------------------------------

	struct EventConnectResult : EventBase
	{
		EventConnectResult(const IEndpoint& remote, EConnectResult res):
			EventBase(remote),
			m_Result(res) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onConnectResult( *m_Network, *m_Endpoint, m_Result );
			});
		}

		EConnectResult m_Result;
	};

	struct EventNewConnection : EventBase
	{
		EventNewConnection(const IEndpoint& remote, const sptr<const IEndpoint> newEtp):
			EventBase(remote),
			m_NewEtp(newEtp) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onNewConnection( *m_Network, *m_Endpoint, m_NewEtp.get() );
			});
		}

		sptr<const IEndpoint> m_NewEtp;
	};

	struct EventDisconnect : EventBase
	{
		EventDisconnect(const IEndpoint& remote, EDisconnectReason reason, const sptr<const IEndpoint>& discEtp):
			EventBase(remote),
			m_DiscEtp(discEtp),
			m_Reason(reason) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onDisconnect( *m_Network, *m_Endpoint, m_Reason, m_DiscEtp.get() );
			});
		}

		sptr<const IEndpoint> m_DiscEtp;
		EDisconnectReason m_Reason;
	};


	// ------ Rpc --------------------------------------------------------------------------------

	/* New connection and disconnect. */

	// [ endpoint : newEtp ]
	MM_RPC( linkStateNewConnection, sptr<IEndpoint> )
	{
		RPC_BEGIN();
		link->pushEvent<EventNewConnection>( get<0>(tp) );
		LOG( "New incoming connection to %s.", link->info() );
	}

	// [ endpoint : discEtp,  bool : wasKicked ]
	MM_RPC( linkStateDisconnect, sptr<IEndpoint>, bool )
	{
		RPC_BEGIN();
		EDisconnectReason reason = get<1>(tp) ? EDisconnectReason::Kicked : EDisconnectReason::Closed;
		link->pushEvent<EventDisconnect>( reason, get<0>(tp) );
		LOG( "Link to %s ignored, already connected.", link->info() );
	}

	/* Connection results */

	MM_RPC( linkStateAlreadyConnected )
	{
		RPC_BEGIN();
		link->pushEvent<EventConnectResult>( EConnectResult::AlreadyConnected );
		LOG( "Link to %s ignored, already connected.", link->info() );
	}

	MM_RPC( linkStatePasswordFail )
	{
		RPC_BEGIN();
		link->pushEvent<EventConnectResult>( EConnectResult::InvalidPassword );
		LOG( "Link to %s ignored, invalid password.", link->info() );
	}

	MM_RPC( linkStateMaxClientsReached )
	{
		RPC_BEGIN();
		link->pushEvent<EventConnectResult>( EConnectResult::MaxConnectionsReached );
		LOG( "Link to %s ignored, max connections reached.", link->info() );
	}

	MM_RPC( linkStateAccepted )
	{
		RPC_BEGIN();
		link->pushEvent<EventConnectResult>( EConnectResult::Fine );
		LOG( "Link to %s accepted.", link->info() );
	}

	/* Incoming connect atempt */

	// [Pw, meta]
	MM_RPC( linkStateConnect, string, MetaData )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( sc<const Endpoint&>(*etp) );
		if ( !link ) return;
			
		// Early out: if linkstate is added, it was already connected.
		if ( link->has<LinkState>() )
		{
			link->callRpc<linkStateAlreadyConnected>();
			return;
		}

		auto originator = link->originator();
		if ( !originator )
		{
			LOGC( "Found link has no originator, invalid behaviour detected!." );
			return;
		}

		// Password fail
		const string& pw = get<0>(tp);
		if ( originator->getPassword() != pw )
		{
			link->callRpc<linkStatePasswordFail>();
			return;
		}
			
		// Max clients reached
		if ( originator->getNumClients() >= originator->getMaxClients() )
		{
			link->callRpc<linkStateMaxClientsReached>();
			return;
		}

		// All fine, send accepted 
		bool wasAccepted = link->getOrAdd<LinkState>()->acceptConnect();
		if ( wasAccepted )
		{
			link->callRpc<linkStateAccepted>(); // To recipient directly
		//	nw.callRpc2<linkStateNewConnection, sptr<IEndpoint>>( link->remoteEtp().to_ptr_nc() , false, &link->remoteEtp(), true /*excl*/ ); // To all others except recipient
			link->pushEvent<EventNewConnection>( link->remoteEtp().to_ptr_nc() );
		}
		else
		{
			// Note: 'acceptConnect' is mutual exclusive, so if called multiple times from recipient, it will still only accept once.
			link->callRpc<linkStateAlreadyConnected>();
		}
	}

	// ------ LinkState --------------------------------------------------------------------------------

	LinkState::LinkState(Link& link):
		ParentLink(link),
		m_State(EConnectState::Unknown)
	{
	}

	MM_TS bool LinkState::connect(const string& pw, const MetaData& md)
	{
		// Check state
		{
			scoped_spinlock lk(m_StateMutex);
			if ( m_State != EConnectState::Unknown )
			{
				LOGW( "Can only connect if state is set to 'Unknown." );
				return false;
			}
			m_State = EConnectState::Connecting;
		}

		return m_Link.callRpc<linkStateConnect, string, MetaData>( pw, md, false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
	}

	MM_TS bool LinkState::acceptConnect()
	{
		// Check state
		{
			scoped_spinlock lk(m_StateMutex);
			if ( m_State != EConnectState::Unknown )
			{
				LOGW( "Can only accept if state is set to 'Unknown." );
				return false;
			}
			m_State = EConnectState::Connected;
		}

		// Any other state (also if already connected, return false).
		return true;
	}

}
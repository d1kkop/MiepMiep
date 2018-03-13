#include "LinkState.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "Rpc.h"
#include "Listener.h"
#include "NetworkListeners.h"
#include "PerThreadDataProvider.h"


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
		EventNewConnection(const IEndpoint& remote, bool isRelayed):
			EventBase(remote),
			m_IsRelayed(isRelayed) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onNewConnection( *m_Network, *m_Endpoint, m_IsRelayed );
			});
		}

		bool m_IsRelayed;
	};

	struct EventDisconnect : EventBase
	{
		EventDisconnect(const IEndpoint& remote, EDisconnectReason reason, bool isRelayed):
			EventBase(remote),
			m_IsRelayed(isRelayed),
			m_Reason(reason) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onDisconnect( *m_Network, *m_Endpoint, m_Reason, m_IsRelayed );
			});
		}

		bool m_IsRelayed;
		EDisconnectReason m_Reason;
	};


	// ------ Rpc --------------------------------------------------------------------------------

	/* New connection and disconnect. */

	MM_RPC( linkStateNewConnection )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			bool isRelayed = true; // Because comes in through rpc, event is always relayed.
			link->pushEvent<EventNewConnection>( isRelayed );
			LOG( "New incoming connection to %s (id %d).", link->ipAndPort(), link->id() );
		}
	}

	// [ bool : wasKicked ]
	MM_RPC( linkStateDisconnect, bool )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			bool isRelayed = (*etp == link->remoteEtp()); // If etp is not remoteEtp, then msg is relayed.
			EDisconnectReason reason = get<0>(tp) ? EDisconnectReason::Kicked : EDisconnectReason::Closed;
			link->pushEvent<EventDisconnect>( reason );
			LOG( "Link to %s (id %d) ignored, already connected.", link->ipAndPort(), link->id() );
		}
	}

	/* Connection results */

	MM_RPC( linkStateAlreadyConnected )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			link->pushEvent<EventConnectResult>( EConnectResult::AlreadyConnected );
			LOG( "Link to %s (id %d) ignored, already connected.", link->ipAndPort(), link->id() );
		}
	}

	MM_RPC( linkStatePasswordFail )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			link->pushEvent<EventConnectResult>( EConnectResult::InvalidPassword );
			LOG( "Link to %s (id %d) ignored, invalid password.", link->ipAndPort(), link->id() );
		}
	}

	MM_RPC( linkStateMaxClientsReached )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			link->pushEvent<EventConnectResult>( EConnectResult::MaxConnectionsReached );
			LOG( "Link to %s (id %d) ignored, max connections reached.", link->ipAndPort(), link->id() );
		}
	}

	MM_RPC( linkStateAccepted )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			link->pushEvent<EventConnectResult>( EConnectResult::Fine );
			LOG( "Link to %s (id %d) accepted.", link->ipAndPort(), link->id() );
		}
	}

	/* Incoming connect atempt */

	// [Pw, meta]
	MM_RPC( linkStateConnect, string, MetaData )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
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
			nw.callRpc2<linkStateNewConnection>( false, &link->remoteEtp(), true ); // To all others except recipient
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
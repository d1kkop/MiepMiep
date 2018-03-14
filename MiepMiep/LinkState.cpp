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
		EventConnectResult(const ILink& link, EConnectResult res):
			EventBase(link),
			m_Result(res) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onConnectResult( *m_Link, m_Result );
			});
		}

		EConnectResult m_Result;
	};

	struct EventNewConnection : EventBase
	{
		EventNewConnection(const ILink& link, const IAddress& newAddr):
			EventBase(link),
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
		EventDisconnect(const ILink& link, EDisconnectReason reason, const IAddress& discAddr):
			EventBase(link),
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
		LOG( "Link to %s ignored, already connected.", l.info() );
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
		l.pushEvent<EventConnectResult>( EConnectResult::Fine );
		LOG( "Link to %s accepted.", l.info() );
	}

	/* Incoming connect atempt */

	// [Pw, meta]
	MM_RPC( linkStateConnect, string, MetaData )
	{
		RPC_BEGIN();
			
		// Early out: if linkstate is added, it was already connected. TODO is this correct?
		if ( l.has<LinkState>() )
		{
			l.callRpc<linkStateAlreadyConnected>();
			return;
		}

		auto originator = l.originator();
		if ( !originator )
		{
			LOGC( "Found link has no originator, invalid behaviour detected!." );
			return;
		}

		// Password fail
		const string& pw = get<0>(tp);
		if ( originator->getPassword() != pw )
		{
			l.callRpc<linkStatePasswordFail>();
			return;
		}
			
		// Max clients reached
		if ( originator->getNumClients() >= originator->getMaxClients() )
		{
			l.callRpc<linkStateMaxClientsReached>();
			return;
		}

		// All fine, send accepted 
		bool wasAccepted = l.getOrAdd<LinkState>()->acceptConnect();
		if ( wasAccepted )
		{
			l.callRpc<linkStateAccepted>(); // To recipient directly
			auto addrCopy = l.destination().getCopy();
			nw.callRpc2<linkStateNewConnection, sptr<IAddress>>( addrCopy , false, &l.destination(), true /*excl*/ ); // To all others except recipient
			l.pushEvent<EventNewConnection>( *addrCopy );
		}
		else
		{
			// Note: 'acceptConnect' is mutual exclusive, so if called multiple times from recipient, it will still only accept once.
			l.callRpc<linkStateAlreadyConnected>();
		}
	}

	// ------ LinkState --------------------------------------------------------------------------------

	LinkState::LinkState(Link& link):
		ParentLink(link),
		m_State(ELinkState::Unknown)
	{
	}

	MM_TS bool LinkState::connect(const string& pw, const MetaData& md)
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

		return m_Link.callRpc<linkStateConnect, string, MetaData>( pw, md, false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
	}

	MM_TS bool LinkState::acceptConnect()
	{
		// Check state
		{
			scoped_spinlock lk(m_StateMutex);
			if ( m_State != ELinkState::Unknown )
			{
				LOGW( "Can only accept if state is set to 'Unknown." );
				return false;
			}
			m_State = ELinkState::Connected;
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
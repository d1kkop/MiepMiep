#include "LinkState.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "Rpc.h"
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


	// ------ Rpc --------------------------------------------------------------------------------

	MM_RPC( linkStateAlreadyConnected )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			link->pushEvent<EventConnectResult>( EConnectResult::AlreadyConnected );
		}
	}

	MM_RPC( linkStateConnect, string, MetaData )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( static_cast<const Endpoint&>(*etp) );
		if ( link )
		{
			if ( link->has<LinkState>() )
			{
				link->callRpc<linkStateAlreadyConnected>();
				return;
			}
		}
	}

	// ------ LinkState --------------------------------------------------------------------------------

	LinkState::LinkState(Link& link):
		ParentLink(link),
		m_State(EConnectState::Unknown)
	{
	}

	bool LinkState::connect(const string& pw, const MetaData& md)
	{
		if ( m_State != EConnectState::Unknown )
		{
			Platform::log(ELogType::Warning, MM_FL, "Can only connect if state is set to 'Unknown'.");
			return false;
		}

		m_State = EConnectState::Connecting;
		m_SharedState = (u32)m_State;

		return m_Link.callRpc<linkStateConnect, string, MetaData>( pw, md, false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
	}

	void LinkState::acceptConnect()
	{
	}

}
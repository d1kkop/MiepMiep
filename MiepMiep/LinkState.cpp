#include "LinkState.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "Rpc.h"
#include "NetworkListeners.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
	// ------ Rpc --------------------------------------------------------------------------------

	MM_RPC( linkStateAlreadyConnected )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( *etp );
		if ( link )
		{
			link->pushEvent<EventConnectResult>( EConnectResult::AlreadyConnected );
		}
	}

	MM_RPC( linkStateConnect )
	{
		auto& nw = toNetwork(network);
		sptr<Link> link = nw.getLink( *etp );
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
		// TODO
		// m_Parent.createGroup(typeId());
	}

	bool LinkState::connect()
	{
		if ( m_State != EConnectState::Unknown )
		{
			Platform::log(ELogType::Warning, MM_FL, "Can only connect if state is set to 'Unknown'.");
			return false;
		}


		m_State = EConnectState::Connecting;
		m_SharedState = (u32)m_State;

		return m_Link.callRpc<linkStateConnect>(false, false, MM_RPC_CHANNEL, nullptr) == ESendCallResult::Fine;
	}

	void LinkState::acceptConnect()
	{

	}

}
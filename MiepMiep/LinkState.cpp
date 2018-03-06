#include "LinkState.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "Rpc.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
	// ------ Rpc --------------------------------------------------------------------------------

	MM_RPC( linkStateConnect )
	{
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

		return true;
	}

	void LinkState::acceptConnect()
	{

	}

}
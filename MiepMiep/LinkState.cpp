#include "LinkState.h"
#include "GroupCollection.h"
#include "Platform.h"
#include "LinkManager.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
	LinkState::LinkState(Link& link):
		Parent(link),
		m_State(EConnectState::Unknown)
	{
		m_Parent.createGroup(typeId());
	}

	void LinkState::createFromRemoteCall(INetwork& network, const IEndpoint& etp)
	{
		sptr<Link> l = static_cast< Network& >( network ).getOrAdd<LinkManager>()->getLink( etp );
		if ( l )
		{
			l->getOrAdd<LinkState>(); 
		}
	}

	bool LinkState::connect()
	{
		if ( m_State != EConnectState::Unknown )
		{
			Platform::log(ELogType::Warning, MM_FL, "Can only connect if state is set to 'Unknown'.");
			return false;
		}

		
		//m_Link.getOrAdd<GroupCollection>(0, m_Link)->addGroup();

		m_State = EConnectState::Connecting;
		return true;
	}

}
#include "NetworkListeners.h"
#include "Endpoint.h"
#include "Network.h"
#include "Link.h"
#include <algorithm>


namespace MiepMiep
{
	NetworkListeners::NetworkListeners(Network& network, bool allowAsyncCallbacks):
		ParentNetwork(network),
		m_AllowAsyncCallbacks( allowAsyncCallbacks )
	{
	}

	MM_TS void NetworkListeners::pushEvent(sptr<IEvent>& event)
	{
		event->m_NetworkListener = this;
		event->m_Network = &m_Network;

		// In this case, immediately async call callback, otherwise push in list and have some other thread
		// consume the events.
		if ( m_AllowAsyncCallbacks || event->m_IsSystemEvent )
		{
			event->process();
		}
		else
		{
			scoped_lock lk(m_EventsMutex);
			m_Events.emplace_back( event );
		}
	}

	MM_TS void NetworkListeners::processAll()
	{
		scoped_lock lk(m_EventsMutex);
		for ( auto& e : m_Events )
		{
			e->process();
		}
		m_Events.clear();
	}


	// --------- EventBase --------------------------------------------------------------------------------------

	EventBase::EventBase(const Link& link, bool isSystemEvent):
		IEvent(isSystemEvent),
		m_Link(link.to_ptr())
	{
	}

}
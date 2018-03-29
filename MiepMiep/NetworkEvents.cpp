#include "NetworkEvents.h"
#include "Network.h"
#include "Link.h"


namespace MiepMiep
{
	NetworkEvents::NetworkEvents(Network& network, bool allowAsyncCallbacks):
		ParentNetwork(network),
		m_AllowAsyncCallbacks( allowAsyncCallbacks )
	{
	}

	MM_TS void NetworkEvents::pushEvent(sptr<IEvent>& event)
	{
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

	MM_TS void NetworkEvents::processQueuedEvents()
	{
		scoped_lock lk(m_EventsMutex);
		for ( auto& e : m_Events )
		{
			e->process();
		}
		m_Events.clear();
	}
}
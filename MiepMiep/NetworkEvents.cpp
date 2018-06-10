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
			rscoped_lock lk(m_EventsMutex);
			m_Events.emplace_back( event );
		}
	}

	MM_TS void NetworkEvents::processQueuedEvents()
	{
		rscoped_lock lk(m_EventsMutex);
		// Doing it this way, new events can be pushed while iterating over.
		while ( !m_Events.empty() )
		{
			m_Events.front()->process();
			m_Events.pop_front();
		}
	}
}
#include "NetworkListeners.h"
#include "Endpoint.h"
#include "Network.h"
#include <algorithm>


namespace MiepMiep
{
	NetworkListeners::NetworkListeners(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS void NetworkListeners::pushEvent(sptr<IEvent>& event)
	{
		event->m_NetworkListener = this;
		event->m_Network = &m_Network;

		// In this case, immediately async call callback, otherwise push in list and have some other thread
		// consume the events.
		if ( m_Network.allowAsyncCallbacks() || event->m_IsSystemEvent )
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

	EventBase::EventBase(const ILink& link):
		m_Link(link.to_ptr())
	{

	}

}
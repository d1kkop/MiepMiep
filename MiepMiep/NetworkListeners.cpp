#include "NetworkListeners.h"
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
		scoped_lock lk(m_EventsMutex);
		m_Events.emplace_back( event );
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

}
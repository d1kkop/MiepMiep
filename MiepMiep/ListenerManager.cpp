#include "ListenerManager.h"
#include "Listener.h"
#include "MiepMiep.h"
#include "Socket.h"


namespace MiepMiep
{
	ListenerManager::ListenerManager(Network& network):
		ParentNetwork(network)
	{
	}

	ListenerManager::~ListenerManager()
	= default;

	MM_TS EListenCallResult ListenerManager::startListen( u16 port )
	{
		sptr<Listener> listener = Listener::startListen( m_Network, port );
		scoped_lock lk(m_ListenersMutex);
		if ( listener )
		{
			assert( m_Listeners.count( port ) == 0 );
			m_Listeners[ port ] = listener;
			m_ListenerSockets[ listener->socket().to_sptr() ] = listener;
			return EListenCallResult::Fine;
		}
		return EListenCallResult::SocketError;
	}

	MM_TS void ListenerManager::stopListen( u16 port )
	{
		scoped_lock lk( m_ListenersMutex );
		auto lIt = m_Listeners.find( port );
		if ( lIt != m_Listeners.end() )
		{
			lIt->second->stopListen();
			m_ListenerSockets.erase( lIt->second->socket().to_sptr() );
			m_Listeners.erase(lIt);
		}
	}

}
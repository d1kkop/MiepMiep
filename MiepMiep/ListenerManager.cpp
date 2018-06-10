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
		scoped_lock lk(m_ListenersMutex);
		if ( m_Listeners.count( port ) != 0 )
		{
			return EListenCallResult::AlreadyExistsOnPort;
		}
		sptr<Listener> listener = Listener::startListen( m_Network, port );
		if ( listener )
		{
			m_Listeners[ port ] = listener;
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
			if ( lIt->second )
			{
				lIt->second->stopListen();
			}
			m_Listeners.erase(lIt);
		}
	}

	MM_TS sptr<Listener> ListenerManager::findListener( u16 port )
	{
		scoped_lock lk( m_ListenersMutex );
		auto lIt = m_Listeners.find( port );
		if ( lIt != m_Listeners.end() )
		{
			return lIt->second;
		}
		return nullptr;
	}

}
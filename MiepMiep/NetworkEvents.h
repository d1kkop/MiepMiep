#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentNetwork.h"
#include <algorithm>
#include <list>


namespace MiepMiep
{
	struct IEvent;
	class Link;

	// -------- EventListener --------------------------------------------------------------------------------------------

	template <typename T>
	struct EventListener
	{
		MM_TS void addListener( T* listener )
		{
			rscoped_lock lk(m_ListenerMutex);
			m_Listeners.emplace_back( listener );
		}

		MM_TS void removeListener( const T* listener )
		{
			rscoped_lock lk( m_ListenerMutex );
			std::remove( m_Listeners.begin(), m_Listeners.end(), listener );
		}

		MM_TS void forListeners( const std::function<void (T*)>& cb )
		{
			rscoped_lock lk( m_ListenerMutex );
			for ( auto l : m_Listeners )
			{
				cb ( l );
			}
		}

	private:
		mutable rmutex m_ListenerMutex;
		vector<T*> m_Listeners;
	};


	// -------- NetworkEvents --------------------------------------------------------------------------------------------

	class NetworkEvents: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		NetworkEvents(Network& network, bool allowAsyncCallbacks);
		static EComponentType compType() { return EComponentType::NetworkEvents; }

		MM_TS void pushEvent( sptr<IEvent>& event );
		MM_TS void processQueuedEvents();
		
	private:
		rmutex m_EventsMutex; // Recursive, otherwise not possible to push new events while iterating through events.
		list<sptr<IEvent>> m_Events;
		bool m_AllowAsyncCallbacks;
	};


	// -------- Polymorphic Events --------------------------------------------------------------------------------------------

	struct IEvent : public ITraceable
	{
		IEvent(const sptr<Link>& link, bool isSystemEvent):
			m_Link( link ),
			m_IsSystemEvent(false)
		{
		}

		virtual void process() = 0;

		sptr<Link> m_Link;
		bool m_IsSystemEvent;
	};

}
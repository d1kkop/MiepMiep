#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentNetwork.h"
#include <algorithm>


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
			scoped_lock lk(m_ListenerMutex);
			m_Listeners.emplace_back( listener );
		}

		MM_TS void removeListener( const T* listener )
		{
			scoped_lock lk( m_ListenerMutex );
			std::remove( m_Listeners.begin(), m_Listeners.end(), listener );
		}

		MM_TS void forListeners( const std::function<void (T*)>& cb )
		{
			scoped_lock lk( m_ListenerMutex );
			for ( auto l : m_Listeners )
			{
				cb ( l );
			}
		}

	private:
		mutex m_ListenerMutex;
		vector<T*> m_Listeners;
	};


	// -------- NetworkListeners --------------------------------------------------------------------------------------------

	class NetworkListeners: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		NetworkListeners(Network& network, bool allowAsyncCallbacks);
		static EComponentType compType() { return EComponentType::NetworkListeners; }

		// Iterates through all events and calls appropriate listeners.
		// Clears list afterwards.
		MM_TS void processAll();

		template <typename T>
		MM_TS void addListener(T* listener);
		template <typename T>
		MM_TS void removeListener( const T* listener );

		MM_TS void pushEvent( sptr<IEvent>& event );

		// Helpers to go from Template Arg to runtime listener Collection.
		// All function in the listeners collection are thread safe. No additional locks are quired to obtain the listener collections.
		template <typename T>  EventListener<T>& getListenerCollection() { }
		template <> EventListener<IConnectionListener>& getListenerCollection() { return m_ConnectionListeners; }

		template <typename T>
		void processEvents( const std::function<void (T*)>& cb )
		{
			getListenerCollection<T>().forListeners( cb );
		}
		

	private:
		EventListener<IConnectionListener> m_ConnectionListeners;

		mutex m_EventsMutex;
		vector<sptr<IEvent>> m_Events;
		bool m_AllowAsyncCallbacks;
	};


	template <typename T>
	MM_TS void NetworkListeners::addListener(T* listener)
	{
		getListenerCollection<T>().addListener( listener );
	}

	template <typename T>
	MM_TS void NetworkListeners::removeListener(const T* listener)
	{
		getListenerCollection<T>().removeListener( listener );
	}


	// -------- Polymorphic Events --------------------------------------------------------------------------------------------

	struct IEvent : public ITraceable
	{
		IEvent(bool isSystemEvent): 
			m_NetworkListener(nullptr),
			m_Network(nullptr),
			m_IsSystemEvent(false)
		{
		}

		virtual void process() = 0;

		NetworkListeners* m_NetworkListener;
		Network* m_Network;
		bool m_IsSystemEvent;
	};

	struct EventBase : IEvent
	{
		EventBase(const Link& link, bool isSystemEvent);

		sptr<const Link> m_Link;
	};

}
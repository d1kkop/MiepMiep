#pragma once

#include "Network.h"
#include <algorithm>


namespace MiepMiep
{
	struct IEvent;


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
		NetworkListeners(Network& network);
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
		IEvent(): 
			m_NetworkListener(nullptr),
			m_Network(nullptr) { }

		virtual void process() = 0;

		NetworkListeners* m_NetworkListener;
		Network* m_Network;
	};

	struct EventBase : IEvent
	{
		EventBase(const sptr<const IEndpoint>& remote):
			m_Endpoint(remote) { }

		sptr<const IEndpoint> m_Endpoint;
	};

	struct EventConnectResult : EventBase
	{
		EventConnectResult(const sptr<const IEndpoint>& remote, EConnectResult res):
			EventBase(remote),
			m_Result(res) { }

		void process() override
		{
			m_NetworkListener->processEvents<IConnectionListener>( [this] (IConnectionListener* l) 
			{
				l->onConnectResult( *m_Network, *m_Endpoint, m_Result );
			});
		}

		EConnectResult m_Result;
	};
}
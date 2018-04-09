#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class Listener;
	class ISocket;


	class ListenerManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		ListenerManager(Network& network);
		~ListenerManager() override;
		static EComponentType compType() { return EComponentType::ListenerManager; }

		MM_TS EListenCallResult startListen( u16 port );
		MM_TS void stopListen( u16 port );
		MM_TS sptr<Listener> findListener( u16 port );

	private:
		mutable mutex m_ListenersMutex;
		map<u16, sptr<Listener>> m_Listeners;
		map<sptr<const ISocket>, sptr<Listener>> m_ListenerSockets;
	};
}
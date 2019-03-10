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

		EListenCallResult startListen( u16 port );
		void stopListen( u16 port );
		sptr<Listener> findListener( u16 port );

	private:
		map<u16, sptr<Listener>> m_Listeners;
	};
}
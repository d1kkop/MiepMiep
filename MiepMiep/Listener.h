#pragma once

#include "Component.h"
#include "ParentNetwork.h"
#include "Memory.h"


namespace MiepMiep
{
	class ISocket;
    class Network;


	class Listener: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		Listener( Network& network );
		~Listener() override;

		static sptr<Listener> startListen( Network& network, u16 port );
		static EComponentType compType() { return EComponentType::Listener; }

		const ISocket&  socket() const { return *m_Socket; }
		const IAddress& source() const { return *m_Source; }
		u16 port() const;

		void stopListen();

		MM_TO_PTR( Listener )

	private:
		MM_TSC bool startListenIntern( u16 port );

	private:
		mutex m_ListeningMutex;
		sptr<ISocket>  m_Socket;
		sptr<IAddress> m_Source;
		bool m_Listening;
	};
}
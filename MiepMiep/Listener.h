#pragma once

#include "Component.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	class ISocket;


	class Listener: public IComponent, public IPacketHandler
	{
	public:
		/* Use Start Listen instead of constructor !! */
		Listener( Network& network );
		~Listener() override;

		MM_TS static sptr<Listener> startListen( Network& network, u16 port );
		MM_TS static EComponentType compType() { return EComponentType::Listener; }

		MM_TSC const ISocket&  socket() const { return *m_Socket; }
		MM_TSC const IAddress& source() const { return *m_Source; }
		MM_TSC u16 port() const;

		MM_TS void stopListen();

		MM_TO_PTR( Listener )

	private:
		MM_TSC bool startListenIntern( u16 port );

	private:
		mutex m_ListeningMutex;
		sptr<ISocket> m_Socket;
		sptr<IAddress> m_Source;
		bool m_Listening;
	};
}
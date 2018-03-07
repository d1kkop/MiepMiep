#pragma once

#include "Network.h"
#include "Threading.h"
#include "Socket.h"


namespace MiepMiep
{
	// -------- NetworkListeners --------------------------------------------------------------------------------------------

	class Listener: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		Listener(Network& network);
		~Listener() override;
		static EComponentType compType() { return EComponentType::Listener; }

		bool startOrRestartListening( u16 port );
		void stopListen();

		MM_TS void setMaxConnections( u32 num );
		MM_TS void setPassword( string& pw );

	private:
		mutex m_ListeningMutex;
		sptr<ISocket> m_Socket;
		bool m_Listening;
		u16 m_ListenPort;
		SpinLock m_PropertiesLock;
		u32 m_MaxConnections;
		string m_Password;
	};
}
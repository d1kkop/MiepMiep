#pragma once

#include "Network.h"
#include "Threading.h"
#include "Socket.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	// -------- NetworkListeners --------------------------------------------------------------------------------------------

	class Listener: public virtual ParentNetwork, public IComponent, public IPacketHandler
	{
	public:
		Listener(Network& network);
		~Listener() override;
		static EComponentType compType() { return EComponentType::Listener; }

		MM_TS bool startOrRestartListening( u16 port );
		MM_TS void stopListen();

		MM_TS void setMaxConnections( u32 num );
		MM_TS void setPassword( string& pw );

	private:
		mutex m_ListeningMutex;
		sptr<ISocket> m_Socket;
		bool m_Listening;
		u16  m_ListenPort;
		atomic_uint m_MaxConnections;
		SpinLock m_PasswordLock;
		string m_Password;
	};
}
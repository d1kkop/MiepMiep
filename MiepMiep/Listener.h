#pragma once

#include "Component.h"
#include "Threading.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	class ISocket;


	class Listener: public IComponent, public IPacketHandler
	{
	public:
		Listener(Network& network);
		~Listener() override;
		static EComponentType compType() { return EComponentType::Listener; }

		MM_TS bool startOrRestartListening( u16 port );
		MM_TS void stopListen();

		MM_TS void setMaxConnections( u32 num );
		MM_TS void setPassword( const string& pw );

		// All packets are handled here first after they have been converted from raw to binStream.
		MM_TS void handleSpecial( class BinSerializer& bs, const Endpoint& etp ) override;

		// These are thread safe because they are either set only from constructor once or they are atomic values.
		MM_TS const ISocket& socket() const { return *m_Socket; }
		MM_TS u16 getPort() const{ return m_ListenPort; }
		MM_TS u32 getMaxClients() const { return m_MaxConnections; }
		MM_TS u32 getNumClients() const { return m_NumConnections; }
		MM_TS string getPassword() const;

		// Called upon link closes so that it unregisters itself.
		MM_TS void reduceNumClientsByOne();

		MM_TO_PTR( Listener )

	private:
		mutex m_ListeningMutex;
		sptr<ISocket> m_Socket;
		bool m_Listening;
		atomic_ushort  m_ListenPort;
		atomic_uint m_MaxConnections;
		atomic_uint m_NumConnections;
		mutable SpinLock m_PasswordLock;
		string m_Password;
	};
}
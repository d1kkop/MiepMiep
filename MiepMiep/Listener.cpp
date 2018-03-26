#include "Listener.h"
#include "Network.h"
#include "SocketSetManager.h"
#include "LinkManager.h"
#include "Endpoint.h"
#include "Socket.h"


namespace MiepMiep
{
	Listener::Listener(Network& network):
		IPacketHandler(network),
		m_Listening(false)
	{
	}

	Listener::~Listener()
	{
		stopListen();
	}

	MM_TS sptr<Listener> Listener::startListen( Network& network, u16 port )
	{
		sptr<Listener> listener = reserve_sp<Listener, Network&>( MM_FL, network );
		if ( listener->startListenIntern( port ) )
			return listener;
		return nullptr;
	}

	MM_TS void Listener::stopListen()
	{
		LOG( "Stop listen was called. Old listen port was %d. ", m_Source?m_Source->port() : 0 );
		scoped_lock lk(m_ListeningMutex);
		if ( m_Socket )
		{
			auto ss = m_Network.get<SocketSetManager>();
			if ( ss )
			{
				ss->removeSocket( m_Socket );
			}
			m_Socket.reset();
			m_Listening = false;
		}
	}

	MM_TSC bool Listener::startListenIntern( u16 port )
	{
		assert( !m_Source && !m_Listening );

		m_Socket = ISocket::create();
		if ( !m_Socket )
		{
			LOGW( "Socket creation failed. ");
			return false;
		}

		i32 err;
		if ( !m_Socket->open( IPProto::Ipv4, SocketOptions(), &err ) )
		{
			LOGW( "Socket open error %d.", err );
			return false;
		}

		if ( !m_Socket->bind( port, &err ) )
		{
			LOGW( "Socket bind error %d.", err );
			return false;
		}

		m_Source = Endpoint::createSource( *m_Socket, &err );
		if ( !m_Source )
		{
			LOGW( "Failed retrieve bound address from socket, error %d.", err );
			return false;
		}

		m_Network.getOrAdd<SocketSetManager>()->addSocket( const_pointer_cast<const ISocket>(m_Socket), ptr<IPacketHandler>() );
		m_Listening = true;
		
		return true;
	}

	MM_TS u16 Listener::port() const
	{
		return m_Source? m_Source->port() : 0;
	}

	MM_TO_PTR_IMP( Listener )

}
#include "Listener.h"
#include "JobSystem.h"
#include "SocketSetManager.h"
#include "LinkManager.h"


namespace MiepMiep
{
	Listener::Listener(Network& network):
		ParentNetwork(network),
		IPacketHandler(network),
		m_ListenPort(-1),
		m_Listening(false)
	{
	}

	Listener::~Listener()
	{
		stopListen();
	}

	MM_TS bool Listener::startOrRestartListening(u16 port)
	{
		scoped_lock lk(m_ListeningMutex);

		if ( port == m_ListenPort && m_Listening )
			return true; // already listening on that port

		// already listening
		if ( m_Listening ) 
		{
			LOG( "Was listening on port %d. But start listening was called with new port: %d.", m_ListenPort, port );
			stopListen();
		}

		m_Socket = ISocket::create();
		if ( m_Socket )
		{
			i32 err;
			if (m_Socket->open(IPProto::Ipv4, false, &err))
			{
				if ( m_Socket->bind( port, &err ) )
				{
					sptr<IPacketHandler> handler = ptr<IPacketHandler>();
					m_Network.getOrAdd<SocketSetManager>()->addSocket( const_pointer_cast<const ISocket>( m_Socket ), handler );
					m_Listening = true;
				}
				else
				{
					LOGW( "Failed to bind socket on port %d, reason: %d.", port, err );
				}
			}
			else
			{
				LOGW( "Failed to open socket on port %d, reason: %d.", port, err );
			}
		}
		
		return false;
	}

	void Listener::stopListen()
	{
		LOG( "Stop listen was called. Old listen port was %d. ", m_ListenPort );
		scoped_lock lk(m_ListeningMutex);
		if ( m_Socket )
		{
			// Remove socket from socket set
			m_Socket.reset();
			m_Listening  = false;
			m_ListenPort = -1;
		}
	}

	MM_TS void Listener::setMaxConnections(u32 num)
	{
		m_MaxConnections = num;
	}

	MM_TS void Listener::setPassword(string& pw)
	{
		scoped_spinlock lk(m_PasswordLock);
		m_Password = pw;
	}

	void Listener::handleSpecial(class BinSerializer& bs, const Endpoint& etp)
	{
		u32 linkId;
		__CHECKED( bs.read(linkId) );

		sptr<Link> link = m_Network.getLink( etp );
		if ( !link )
		{
			// is new link
			link = m_Network.getOrAdd<LinkManager>()->addLink( etp );
			link->setOriginator( *this );
		}

		link->receive( bs );
	}
}
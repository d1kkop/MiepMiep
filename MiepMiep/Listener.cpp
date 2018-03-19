#include "Listener.h"
#include "Network.h"
#include "Link.h"
#include "JobSystem.h"
#include "SocketSetManager.h"
#include "LinkManager.h"
#include "Endpoint.h"
#include "Socket.h"


namespace MiepMiep
{
	Listener::Listener(Network& network):
		IPacketHandler(network),
		m_ListenPort(-1),
		m_Listening(false)
	{
	}

	Listener::~Listener()
	{
		stopListen();
	}

	MM_TS sptr<Link> Listener::getOrCreateLinkFrom( u32 linkId, const SocketAddrPair& sap )
	{
		auto lm = m_Network.getOrAdd<LinkManager>();
		sptr<Link> link = lm->get( sap );
		if ( !link )
		{
			link = lm->add( linkId, *this, *sap.m_Address );
			if ( !link )
			{
				LOGW( "Failed to add link to %s.", sap.m_Address->toIpAndPort() );
			}
		}
		return link;
	}

	MM_TS bool Listener::startOrRestartListening( u16 port )
	{
		scoped_lock lk(m_ListeningMutex);

		if ( port == m_ListenPort && m_Listening )
		{
			LOGW( "Started listener while already listening on requested port %d, call ignored.", port );
			return true; // already listening on that port
		}

		// already listening
		if ( m_Listening ) 
		{
			LOG( "Was listening on port %d. But start listening was called with new port: %d. This is ok, though it would be cleaner to call 'stopListen' first.",
				 (u16)m_ListenPort, port );
			stopListen();
		}

		m_Socket = ISocket::create();
		if ( m_Socket )
		{
			i32 err;
			if (m_Socket->open(IPProto::Ipv4, SocketOptions(), &err))
			{
				if ( m_Socket->bind( port, &err ) )
				{
					m_Source = Endpoint::createSource( *m_Socket, &err );
					if ( m_Source )
					{
						sptr<IPacketHandler> handler = ptr<IPacketHandler>();
						m_Network.getOrAdd<SocketSetManager>()->addSocket( const_pointer_cast<const ISocket>( m_Socket ), handler );
						m_Listening = true;
						LOG( "Started listening on port %d.", port );
					}
					else
					{
						LOGW( "Failed to determine local bound address, error: %d.", err );
					}
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
		LOG( "Stop listen was called. Old listen port was %d. ", (u16)m_ListenPort );
		scoped_lock lk(m_ListeningMutex);
		if ( m_Socket )
		{
			auto ss = m_Network.get<SocketSetManager>();
			if ( ss )
			{
				ss->removeSocket( m_Socket );
			}
			m_Socket.reset();
			m_Listening  = false;
			m_ListenPort = -1;
		}
	}

	MM_TS void Listener::setMaxConnections(u32 num)
	{
		m_MaxConnections = num;
	}

	MM_TS void Listener::setPassword(const string& pw)
	{
		scoped_spinlock lk(m_PasswordLock);
		m_Password = pw;
	}

	MM_TS string Listener::getPassword() const
	{
		scoped_spinlock lk(m_PasswordLock);
		return m_Password;
	}

	MM_TS void Listener::increaseNumClientsByOne()
	{
		assert(m_NumConnections != (u32)~0);
		m_NumConnections++;
	}

	MM_TS void Listener::reduceNumClientsByOne()
	{
		m_NumConnections--;
		assert( m_NumConnections != (u32)~0 );
	}

	MM_TO_PTR_IMP( Listener )
}
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

	MM_TS bool Listener::startOrRestartListening(u16 port)
	{
		scoped_lock lk(m_ListeningMutex);

		if ( port == m_ListenPort && m_Listening )
		{
			LOG( "Started listener while already listening on requested port %d, call ignored.", port );
			return true; // already listening on that port
		}

		// already listening
		if ( m_Listening ) 
		{
			LOG( "Was listening on port %d. But start listening was called with new port: %d.", (u16)m_ListenPort, port );
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

	MM_TS void Listener::reduceNumClientsByOne()
	{
		m_NumConnections--;
		assert( m_NumConnections != (u32)~0 );
	}

	MM_TS void Listener::handleSpecial(class BinSerializer& bs, const Endpoint& etp)
	{
		u32 linkId;
		__CHECKED( bs.read(linkId) );

		bool added;
		sptr<Link> link = m_Network.getOrAdd<LinkManager>()->getOrAdd( etp, &linkId, this, &added );
		if ( !link )
		{
			LOGW( "Failed to add link to %s.", etp.toIpAndPort() );
			return;
		}

		// If first time added, increment num connections on listener.
		// If link leaves, it will reduce the count by 1 from its destructor.
		if ( added )
		{
			m_NumConnections++;
		}

		if ( link->id() == linkId )
		{
			link->receive( bs );
		}
		else
		{
			LOGW( "Listener: Packet for link was discarded as link id's did not match." );
		}
	}


	MM_TO_PTR_IMP( Listener )
}
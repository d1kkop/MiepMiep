#include "SocketSetManager.h"
#include "Socket.h"
#include "Link.h"
#include "Util.h"
#include "Network.h"
#include "Endpoint.h"


namespace MiepMiep
{
	// -------- ReceptionThread -------------------------------------------------------------------------------------

	ReceptionThread::ReceptionThread(SocketSetManager& manager):
		m_Manager(manager),
		m_Network(manager.m_Network)
	{
	}

	ReceptionThread::~ReceptionThread()
	{
		stop();
	}

	MM_TS bool ReceptionThread::addSocket(const sptr<const ISocket>& sock)
	{
		scoped_lock lk(m_HighLevelSocketsMutex);
	#if MM_SDLSOCKET
	#error no implementation
	#else MM_WIN32SOCKET
		if ( m_HighLevelSockets.size() >= FD_SETSIZE ) return false; // Cannot add socket to this set. Create new set.
		m_HighLevelSockets[sc<const BSDSocket&>( *sock ).getSock()] = sock;
	#endif
		m_IsDirty = true;
		return true;
	}

	MM_TS void ReceptionThread::removeSocket(const sptr<const ISocket>& sock)
	{
		scoped_lock lk(m_HighLevelSocketsMutex);
		m_IsDirty = true;
	#if MM_SDLSOCKET
	#error no implementation
	#else MM_WIN32SOCKET
		m_HighLevelSockets.erase( sc<const BSDSocket&>( *sock ).getSock() );
	#endif
	}

	void ReceptionThread::start()
	{
		m_Thread = thread( [this]() 
		{
			receptionThread();
		});
	}

	void ReceptionThread::stop()
	{
		m_HighLevelSocketsMutex.lock();
		m_Closing = true;
	#if MM_SDLSOCKET
	#error no implementation
	#else MM_WIN32SOCKET
		for ( u_int i = 0; i < m_LowLevelSocketArray.fd_count; i++ )
		{
			closesocket( m_LowLevelSocketArray.fd_array[i] );
		}
	#endif
		m_HighLevelSocketsMutex.unlock();
		if ( m_Thread.joinable() )
		{
			m_Thread.join();
		}
	}

	void ReceptionThread::receptionThread()
	{
		set_terminate([]()
		{ 
			LOGC( "ReceptionThread encountered exception." );
			abort();
		});

		while ( true )
		{
			i32 err;
			EListenOnSocketsResult res = listenOnSockets( m_Manager.m_Network, MM_SOCK_SELECT_TIMEOUT, &err );

			if ( m_Closing )
				break;

			if ( err != 0 )
			{
				LOGW( "Listen on socket set returned error %d.", err );
				this_thread::sleep_for( 1ms ); // In case of error wait 1 ms to avoid a spinning while loop if contineously errors occur.
				continue;
			}
		}
	}

	void ReceptionThread::rebuildSocketArrayIfNecessary()
	{

	#if MM_SDLSOCKET
	#error no implementation
	#elif MM_WIN32SOCKET

		scoped_lock lk( m_HighLevelSocketsMutex );
		// TODO -> Apparently, we have to rebuild the set each time again.
		m_IsDirty = true;

		if ( m_IsDirty )
		{
			FD_ZERO( &m_LowLevelSocketArray );
			assert( m_HighLevelSockets.size() <= FD_SETSIZE );

			for ( auto& kvp : m_HighLevelSockets )
			{
				SOCKET s = sc<const BSDSocket&>( *kvp.second ).getSock();
				FD_SET( s, &m_LowLevelSocketArray );
			}

			m_LowLevelSocketArray.fd_count = (u_int)m_HighLevelSockets.size();
			m_IsDirty = false;
		}

	#endif

	}

	void ReceptionThread::handleReceivedPacket( Network& network, const ISocket& sock )
	{
		byte* buff = reserveN<byte>( MM_FL, MM_MAX_RECVSIZE );
		u32 rawSize = MM_MAX_RECVSIZE; // buff size in
		Endpoint etp;
		i32 err;
		ERecvResult res = sock.recv( buff, rawSize, etp, &err );

		u32 packetLossPercentage = m_Network.packetLossPercentage();
		if ( packetLossPercentage != 0 && (Util::rand() % 100) + 1 <= packetLossPercentage )
		{
			// drop deliberately
			return;
		}

	#if _DEBUG
		etp.m_HostPort = etp.getPortHostOrder();
	#endif

		//	LOG( "Received data from.. %s.", etp.toIpAndPort() );

		if ( err != 0 )
		{
			LOG( "Socket recv error: %d.", err );
			releaseN( buff );
			return;
		}

		if ( res == ERecvResult::Succes )
		{
			if ( rawSize >= MM_MIN_HDR_SIZE )
			{
				// TODO Continue here
				//auto am = network.getOrAdd<NetworkActions>();
				//	am->addAction( reserve_sp<

				auto lm = network.getOrAdd<LinkManager>();
				sptr<Link> link = lm->getOrAdd( nullptr, SocketAddrPair( sock, *etp.getCopyDerived() ), nullptr );
				if ( link )
				{
					BinSerializer bs( buff, MM_MAX_RECVSIZE, rawSize, false, false );
					link->receive( bs );
				}

				//m_Network.get<JobSystem>()->addJob(
				//	[p = move( make_shared<RecvPacket>( 0, buff, rawSize, 0, false ) ),
				//	ph = move( ptr<PacketHandler>() ),
				//	e  = etp.getCopyDerived(),
				//	s  = sock.to_sptr()]
				//{
				//	BinSerializer bs( p->m_Data, MM_MAX_RECVSIZE, p->m_Length, false, false );
				//	ph->handleInitialAndPassToLink( bs, *s, *e );
				//} );

				//// passed to thread-job
			}
			else
			{
				LOGW( "Received packet with less than %d bytes (= Hdr size), namely %d. Packet discarded.", MM_MIN_HDR_SIZE, rawSize );
			}
		}

		releaseN( buff );
	}

	EListenOnSocketsResult ReceptionThread::listenOnSockets( Network& network, u32 timeoutMs, i32* err )
	{
		if ( err ) *err = 0;
		rebuildSocketArrayIfNecessary();

		if ( m_LowLevelSocketArray.fd_count == 0 )
			return EListenOnSocketsResult::NoSocketsInSet;

	#if MM_SDLSOCKET
	#error no implementation
	#elif MM_WIN32SOCKET

		// Query sockets for data
		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = timeoutMs * 1000 * MM_SOCK_SELECT_TIMEOUT;

		// NOTE: select reorders the ready sockets from 0 to end-1, and fd_count is adjusted to the num of ready sockets.
		i32 res = select( 0, &m_LowLevelSocketArray, nullptr, nullptr, &tv );
		if ( res < 0 )
		{
			if ( err ) *err = GetLastError();
			return EListenOnSocketsResult::Error;
		}

		// timed out (no data available)
		if ( res == 0 )
		{
			return EListenOnSocketsResult::TimeoutNoData;
		}

		// at least a single one has data, check which ones and call cb
		for ( i32 i = 0; i < res; i++ )
		{
			SOCKET s = m_LowLevelSocketArray.fd_array[i];

			// Because only checking 'read' sockets, s must be SET.
			assert( FD_ISSET( s, &m_LowLevelSocketArray ) );

			//	LOG( "Recv on ... %ull.", s ); 

			// Only use lock for obtaining socket, then release as there is no need to keep lock.
			sptr<const ISocket> hSocket;
			{
				scoped_lock lk( m_HighLevelSocketsMutex );
				auto sockIt = m_HighLevelSockets.find( s );
				if ( sockIt != m_HighLevelSockets.end() )
				{
					hSocket = sockIt->second;
				}
			}

			// Start receiving the packet.
			if ( hSocket )
			{
				handleReceivedPacket( network, *hSocket );
			}
		}

	#endif

		return EListenOnSocketsResult::Fine;
	}


	// -------- SocketSetManager -------------------------------------------------------------------------------------

	SocketSetManager::SocketSetManager(Network& network):
		ParentNetwork(network)
	{
	}

	SocketSetManager::~SocketSetManager()
	{
		stop();
	}

	void SocketSetManager::stop()
	{
		m_ReceptionThreads.clear(); // will invoke reception thread destructors which join the calling thread
	}

	void SocketSetManager::addSocket(const sptr<const ISocket>& sock)
	{
		// try all sets
		for ( auto& r : m_ReceptionThreads )
		{
			if ( r->addSocket( sock ) )
			{
				return;
			}
		}

		m_ReceptionThreads.emplace_back( reserve_sp<ReceptionThread, SocketSetManager&>(MM_FL, *this) );
		bool wasAdded = m_ReceptionThreads.back()->addSocket( sock );
		assert( wasAdded );

		m_ReceptionThreads.back()->start();
	}

	void SocketSetManager::removeSocket(const sptr<const ISocket>& sock)
	{
		// try all sets
		for ( auto& r : m_ReceptionThreads )
		{
			// could be added multiple times, do not break after first succesful remove
			r->removeSocket( sock );
		}
	}

}
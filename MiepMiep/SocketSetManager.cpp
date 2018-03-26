#include "SocketSetManager.h"
#include "SocketSet.h"


namespace MiepMiep
{
	// -------- ReceptionThread -------------------------------------------------------------------------------------

	ReceptionThread::ReceptionThread(SocketSetManager& manager):
		m_Manager(manager),
		m_SockSet(reserve_sp<SocketSet>(MM_FL))
	{
	}

	ReceptionThread::~ReceptionThread()
	{
		stop();
	}

	bool ReceptionThread::addSocket(const sptr<const ISocket>& sock, const sptr<IPacketHandler>& handler)
	{
		scoped_lock lk(m_SocketSetMutex);
		return m_SockSet->addSocket( sock, handler );
	}

	void ReceptionThread::removeSocket(const sptr<const ISocket>& sock)
	{
		scoped_lock lk(m_SocketSetMutex);
		m_SockSet->removeSocket( sock );
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
			EListenOnSocketsResult res = m_SockSet->listenOnSockets( MM_SOCK_SELECT_TIMEOUT, &err );

			if ( m_Manager.isClosing() )
				break;

			if ( err != 0 )
			{
				LOGW( "Listen on socket set returned error %d.", err );
				continue;
			}
		}
	}

	// -------- SocketSetManager -------------------------------------------------------------------------------------

	SocketSetManager::SocketSetManager(Network& network):
		ParentNetwork(network),
		m_Closing(false)
	{
	}

	SocketSetManager::~SocketSetManager()
	{
		stop();
	}

	void SocketSetManager::stop()
	{
		m_Closing = true;
		m_ReceptionThreads.clear(); // will invoke reception thread destructors which join the calling thread
	}

	MM_TS void SocketSetManager::addSocket(const sptr<const ISocket>& sock, const sptr<IPacketHandler>& handler)
	{
		scoped_lock lk(m_ReceptionThreadsMutex);

		// try all sets
		for ( auto& r : m_ReceptionThreads )
		{
			if ( r->addSocket( sock, handler ) )
			{
				return;
			}
		}

		m_ReceptionThreads.emplace_back( reserve_sp<ReceptionThread, SocketSetManager&>(MM_FL, *this) );
		bool wasAdded = m_ReceptionThreads.back()->addSocket( sock, handler );
		assert( wasAdded );

		m_ReceptionThreads.back()->start();
	}

	MM_TS void SocketSetManager::removeSocket(const sptr<const ISocket>& sock)
	{
		scoped_lock lk(m_ReceptionThreadsMutex);

		// try all sets
		for ( auto& r : m_ReceptionThreads )
		{
			// could be added multiple times, do not break after first succesful remove
			r->removeSocket( sock );
		}
	}

}
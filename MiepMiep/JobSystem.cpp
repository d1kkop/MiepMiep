#include "JobSystem.h"
#include "Platform.h"
#include <cassert>
#include <sstream>


namespace MiepMiep
{
	// ------------ WorkerThread --------------------------------------------------------------------------------

	WorkerThread::WorkerThread(JobSystem& js):
		m_Js(js),
		m_Closing(false)
	{
	}

	WorkerThread::~WorkerThread()
	{
		stop();
	}

	MM_TS void WorkerThread::start()
	{
		set_terminate([]()
		{ 
			LOGC( "Worker thread encountered exception." );
			abort();
		});

		m_Thread = thread([&]
		/* This is the thread function */
		{
			while ( true )
			{
				unique_lock<mutex> lk( m_Js.getMutex() );
				if ( m_Js.isClosing() )
				{
					break;
				}
				Job j = m_Js.extractJob();
				if ( !j.valid() )
				{
					m_Js.suspend( lk );
					j = m_Js.extractJob();
				}
				lk.unlock();
				if ( j.valid() )
				{
				#if MM_TRACE_JOBSYSTEM
					stringstream ss;
					this_thread::get_id()._To_text( ss );
					LOG( "Thread %s starts to execute job.", ss.str().c_str() );
				#endif
					j.m_WorkFunc();
				}
			}
		});
	}

	MM_TS void WorkerThread::stop()
	{
		if ( m_Thread.joinable() )
		{
			m_Thread.join();
		}
	}

	// ------------ JobSystem --------------------------------------------------------------------------------

	JobSystem::JobSystem(Network& network, u32 numWorkerThreads):
		ParentNetwork(network),
		m_Closing(false),
		m_NumThreadsSleeping(0)
	{
		assert( numWorkerThreads != 0 );
		for (u32 i = 0; i < numWorkerThreads ; i++)
		{
			m_WorkerThreads.push_back( make_unique<WorkerThread>(*this) );
			m_WorkerThreads.back()->start();
		}
	}

	JobSystem::~JobSystem()
	{
		stop();
	}

	MM_TS void JobSystem::addJob(const std::function<void()>& cb)
	{
		#if MM_TRACE_JOBSYSTEM
			stringstream ss;
			this_thread::get_id()._To_text( ss );
			LOG( "Thread %s locks.", ss.str().c_str() );
		#endif
		m_JobsMutex.lock();
		m_GlobalQueue.push ( { cb } );
		if ( m_NumThreadsSleeping > 0 )
		{
			m_QueueCv.notify_one();
			m_NumThreadsSleeping--;
		}
		m_JobsMutex.unlock();
		#if MM_TRACE_JOBSYSTEM
			stringstream ss2;
			this_thread::get_id()._To_text( ss2 );
			LOG( "Thread %s unlocks.", ss2.str().c_str() );
		#endif
		//m_QueueCv.notify_one();
	}

	MM_TS Job JobSystem::extractJob()
	{
		if ( !m_GlobalQueue.empty() )
		{
			Job j = m_GlobalQueue.front();
			m_GlobalQueue.pop();
			return j;
		}
		return Job();
	}

	MM_TS void JobSystem::suspend(unique_lock<mutex>& ul)
	{
		#if MM_TRACE_JOBSYSTEM
			stringstream ss;
			this_thread::get_id()._To_text( ss );
			LOG( "Thread %s suspends -> loses lock.", ss.str().c_str() );
		#endif
		m_NumThreadsSleeping++;
		m_QueueCv.wait( ul );
	}

	MM_TS void JobSystem::stop()
	{
		m_JobsMutex.lock();
		m_Closing = true;
		m_QueueCv.notify_all();
		m_JobsMutex.unlock();
		m_WorkerThreads.clear();
		scoped_lock lk(m_JobsMutex);
		while ( !m_GlobalQueue.empty() ) m_GlobalQueue.pop();
	}

}
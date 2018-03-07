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

	void WorkerThread::start()
	{
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
				Job j = m_Js.peekJob();
				if ( !j.valid() )
				{
					m_Js.suspend( lk );
					j = m_Js.peekJob();
				}
				lk.unlock();
				if ( j.valid() )
				{
					j.m_WorkFunc();
				}
			}
		});
	}

	void WorkerThread::stop()
	{
		if ( m_Thread.joinable() )
		{
			m_Thread.join();
		}
	}

	// ------------ JobSystem --------------------------------------------------------------------------------

	JobSystem::JobSystem(Network& network, u32 numWorkerThreads):
		ParentNetwork(network),
		m_Closing(false)
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

	void JobSystem::addJob(const std::function<void()>& cb)
	{
		#if MM_TRACE_JOBSYSTEM
			stringstream ss;
			this_thread::get_id()._To_text( ss );
			LOG( "Thread %s locks.", ss.str().c_str() );
		#endif
		m_JobsMutex.lock();
		m_GlobalQueue.push_back ( { cb } );
		m_JobsMutex.unlock();
		#if MM_TRACE_JOBSYSTEM
			stringstream ss2;
			this_thread::get_id()._To_text( ss2 );
			LOG( "Thread %s unlocks.", ss2.str().c_str() );
		#endif
		m_QueueCv.notify_one();
	}

	Job JobSystem::peekJob()
	{
		if ( !m_GlobalQueue.empty() )
		{
			Job j = m_GlobalQueue.back();
			m_GlobalQueue.pop_back();
			return j;
		}
		return Job();
	}

	void JobSystem::suspend(unique_lock<mutex>& ul)
	{
		#if MM_TRACE_JOBSYSTEM
			stringstream ss;
			this_thread::get_id()._To_text( ss );
			LOG( "Thread %s suspends -> loses lock.", ss.str().c_str() );
		#endif
		m_QueueCv.wait( ul );
	}

	void JobSystem::stop()
	{
		m_JobsMutex.lock();
		m_Closing = true;
		m_QueueCv.notify_all();
		m_JobsMutex.unlock();
		for ( auto& wt :  m_WorkerThreads )
		{
			wt->stop();
		}
		m_WorkerThreads.clear();
		m_GlobalQueue.clear();
	}

}
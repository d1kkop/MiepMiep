#include "JobSystem.h"
#include <cassert>


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
			while (!m_Closing)
			{
				m_Js.lock();
				Job j = m_Js.peekJob();
				if ( !j.valid() )
				{
					m_Js.suspend();
					if ( m_Closing )
					{
						m_Js.unlock();
						break;
					}
					j = m_Js.peekJob();
				}
				m_Js.unlock();
				if ( j.valid() )
				{
					j.m_WorkFunc();
				}
			}
		});
	}

	void WorkerThread::stop()
	{
		m_Closing = true;
		if ( m_Thread.joinable() )
		{
			m_Thread.join();
		}
	}

	// ------------ JobSystem --------------------------------------------------------------------------------

	JobSystem::JobSystem(Network& network, u32 numWorkerThreads):
		ParentNetwork(network)
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
		m_Lock.lock();
		m_GlobalQueue.push_back ( { cb } );
		m_Lock.unlock();
		m_QueueCv.notify_one();
	}

	void JobSystem::lock()
	{
		m_Lock.lock();
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

	void JobSystem::suspend()
	{
		m_QueueCv.wait( m_Lock );
	}

	void JobSystem::unlock()
	{
		m_Lock.unlock();
	}

	void JobSystem::stop()
	{
		for ( auto& wt :  m_WorkerThreads )
		{
			wt->stop();
		}
		m_WorkerThreads.clear();
		m_Lock.lock();
		m_GlobalQueue.clear();
		m_Lock.unlock();
		m_QueueCv.notify_all();
	}

}
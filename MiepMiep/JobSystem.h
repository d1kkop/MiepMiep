#pragma once


#include "Component.h"
#include "Memory.h"
#include <list>
#include <functional>
#include <thread>
#include <condition_variable>
using namespace std;


namespace MiepMiep
{
	struct Job
	{
		bool valid() const { return m_WorkFunc!=nullptr; }
		std::function<void ()> m_WorkFunc;
	};


	class WorkerThread
	{
	public:
		WorkerThread(class JobSystem& js);
		~WorkerThread();
		void start();
		void stop();

		thread m_Thread;
		class JobSystem& m_Js;
		volatile bool m_Closing;
	};


	class JobSystem: public IComponent, public ITraceable
	{
	public:
		JobSystem(u32 numWorkerThreads=3);
		~JobSystem() override;


		void addJob( const std::function<void ()>& cb );

	private:
		void lock();
		Job peekJob();
		void suspend();
		void unlock();


	private:
		mutex m_JobsMutex;
		list<Job> m_GlobalQueue;
		condition_variable m_QueueCv;
		unique_lock<mutex> m_Lock;
		vector<uptr<WorkerThread>> m_WorkerThreads;

		friend class WorkerThread;
	};

}
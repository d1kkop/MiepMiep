#pragma once


#include "Component.h"
#include "Memory.h"
#include "Network.h"
#include <functional>
#include <thread>
#include <queue>
#include <condition_variable>
using namespace std;


namespace MiepMiep
{
	class Network;

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


	class JobSystem: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		JobSystem(Network& network, u32 numWorkerThreads=3);
		~JobSystem() override;
		// IComponent
		static EComponentType compType() { return EComponentType::JobSystem; }


		void addJob( const std::function<void ()>& cb );
		void stop();
		bool isClosing() const volatile { return m_Closing; }

	private:
		Job peekJob();
		mutex& getMutex() { return m_JobsMutex; }
		void suspend(unique_lock<mutex>& ul);


	private:
		volatile bool m_Closing;
		mutex m_JobsMutex;
		queue<Job> m_GlobalQueue;
		condition_variable m_QueueCv;
		vector<uptr<WorkerThread>> m_WorkerThreads;

		friend class WorkerThread;
	};

}
#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class ISocket;
	class SocketSet;
	class SocketSetManager;

	class ReceptionThread: public ITraceable
	{
	public:
		ReceptionThread(SocketSetManager& manager);
		~ReceptionThread() override;
		MM_TS bool addSocket(const sptr<const ISocket>& sock);
		MM_TS void removeSocket(const sptr<const ISocket>& sock);
		void start();
		void stop();
		void receptionThread();

	private:
		SocketSetManager& m_Manager;
		thread m_Thread;
		mutex m_SocketSetMutex;
		sptr<SocketSet> m_SockSet;
	};


	class SocketSetManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		SocketSetManager(Network& network);
		~SocketSetManager() override;
		static EComponentType compType() { return EComponentType::SocketSetManager; }
		bool isClosing() const volatile { return m_Closing; }
		void stop();

		MM_TS void addSocket( const sptr<const ISocket>& sock );
		MM_TS void removeSocket( const sptr<const ISocket>& sock );

	private:
		bool m_Closing;
		mutex m_ReceptionThreadsMutex;
		vector<sptr<ReceptionThread>> m_ReceptionThreads;
	};
}
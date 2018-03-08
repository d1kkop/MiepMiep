#pragma once

#include "Network.h"
#include "SocketSet.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	class SocketSetManager;

	class ReceptionThread: public ITraceable
	{
	public:
		ReceptionThread(SocketSetManager& manager);
		virtual ~ReceptionThread();
		MM_TS bool addSocket(sptr<const ISocket>& sock, const sptr<IPacketHandler>& handler);
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

		MM_TS void addSocket( sptr<const ISocket>& sock, const sptr<IPacketHandler>& handler );
		MM_TS void removeSocket( const sptr<const ISocket>& sock );

	private:
		volatile bool m_Closing;
		mutex m_ReceptionThreadsMutex;
		vector<sptr<ReceptionThread>> m_ReceptionThreads;
	};
}
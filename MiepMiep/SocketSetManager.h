#pragma once

#include "Memory.h"
#include "Platform.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class ISocket;
	class SocketSet;
	class SocketSetManager;

	enum class EListenOnSocketsResult
	{
		Fine,
		TimeoutNoData,
		NoSocketsInSet,
		Error
	};

	class ReceptionThread: public ITraceable
	{
	public:
		ReceptionThread(SocketSetManager& manager);
		~ReceptionThread() override;
		MM_TS bool addSocket(const sptr<const ISocket>& sock);
		MM_TS void removeSocket(const sptr<const ISocket>& sock);
		void start();
		void stop();

		// Called from new thread which will invoke this private members listenOnSockets etc.
		void receptionThread();

	private:
		void rebuildSocketArrayIfNecessary();
		void handleReceivedPacket( Network& network, const ISocket& sock );
		EListenOnSocketsResult listenOnSockets( Network& network, u32 timeoutMs, i32* err );

		SocketSetManager& m_Manager;
		Network& m_Network;
		thread m_Thread;

		bool m_IsDirty;
		bool m_Closing;

	#if MM_SDLSOCKET
	#error no implementation
	#elif MM_WIN32SOCKET
		// Max of 64 for BSD see FD_SETSIZE
		map<SOCKET, sptr<const ISocket>> m_HighLevelSockets;
		fd_set m_LowLevelSocketArray;
	#endif

		mutex m_HighLevelSocketsMutex;
		sptr<SocketSet> m_SockSet;
	};


	class SocketSetManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		SocketSetManager(Network& network);
		~SocketSetManager() override;
		static EComponentType compType() { return EComponentType::SocketSetManager; }
		void stop();

		void addSocket( const sptr<const ISocket>& sock );
		void removeSocket( const sptr<const ISocket>& sock );

	private:
		vector<sptr<ReceptionThread>> m_ReceptionThreads;
	};
}
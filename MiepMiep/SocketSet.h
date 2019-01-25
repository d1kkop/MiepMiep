#pragma once

#include "Memory.h"
#include "Platform.h"
using namespace std;


namespace MiepMiep
{
	class ISocket;
	class Network;


	enum class EListenOnSocketsResult
	{
		Fine,
		TimeoutNoData,
		NoSocketsInSet,
		Error
	};


	class SocketSet: public ITraceable
	{
	public:
		SocketSet();
		~SocketSet() override;

		// Fails if max number of sockets was added. For BSD sockets the default is 64.
		MM_TS bool addSocket(const sptr<const ISocket>& sock);
		MM_TS void removeSocket(const sptr<const ISocket>& sock);
		MM_TS bool hasSocket(const sptr<const ISocket>& sock) const;

		// NOTE: Not thread safe. Should be called from a single thread to listen on sockets.
		EListenOnSocketsResult listenOnSockets(Network& network, u32 timeoutMs, i32* err=nullptr);

	private:
		void rebuildSocketArrayIfNecessary();
        void handleReceivedPacket(Network& network, const ISocket& sock);

	private:
		bool m_IsDirty;
		bool m_Closing;

	#if MM_SDLSOCKET
    #error no implementation
	#elif MM_WIN32SOCKET
		// Max of 64 for BSD see FD_SETSIZE
		mutable mutex m_HighLevelSocketsMutex;
		map<SOCKET, sptr<const ISocket>> m_HighLevelSockets;
		fd_set m_LowLevelSocketArray;
	#endif
	};
}
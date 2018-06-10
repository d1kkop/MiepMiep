#pragma once

#include "Memory.h"
#include "Platform.h"
using namespace std;


namespace MiepMiep
{
	class ISocket;
	class IPacketHandler;


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
		MM_TS bool addSocket(const sptr<const ISocket>& sock, const sptr<IPacketHandler>& packetHandler);
		MM_TS void removeSocket(const sptr<const ISocket>& sock);
		MM_TS bool hasSocket(const sptr<const ISocket>& sock) const;

		// NOTE: Not thread safe. Should be called from a single thread to listen on sockets.
		// Suggestion: Use a seperate thread per SocketSet.
		EListenOnSocketsResult listenOnSockets(u32 timeoutMs, i32* err=nullptr);

	private:
		MM_TS void rebuildSocketArrayIfNecessary();

	private:
		bool m_IsDirty;
		bool m_Closing;
		mutable mutex m_SetMutex;

	#if MM_SDLSOCKET
	#elif MM_WIN32SOCKET
		// Max of 64 for BSD see FD_SETSIZE
		map<SOCKET, pair<sptr<const ISocket>, sptr<IPacketHandler>>> m_HighLevelSockets;
		fd_set m_LowLevelSocketArray;
	#endif
	};
}
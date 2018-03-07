#pragma once

#include "Common.h"
#include "Memory.h"
#include "Socket.h"
using namespace std;


namespace MiepMiep
{
	enum class EListenOnSocketsResult
	{
		Fine,
		TimeoutNoData,
		NoSocketsInSet,
		Error
	};

	using PacketHandler = std::function<void (sptr<const ISocket>&)>;


	class SocketSet: public ITraceable
	{
	public:
		MM_DECLSPEC_INTERN SocketSet();
		MM_DECLSPEC_INTERN ~SocketSet() override;

		// Fails if max number of sockets was added. For BSD sockets the default is 64.
		MM_TS MM_DECLSPEC_INTERN bool addSocket(sptr<const ISocket>& sock, const PacketHandler& packetHandler);
		MM_TS MM_DECLSPEC_INTERN void removeSocket(sptr<const ISocket>& sock);

		// NOTE: Not thread safe. Should be called from a single thread to listen on sockets.
		// Suggestion: Use a seperate thread per SocketSet.
		EListenOnSocketsResult MM_DECLSPEC_INTERN listenOnSockets(u32 timeoutMs, i32* err=nullptr);

	private:
		MM_TS void rebuildSocketArrayIfNecessary();

	private:
		mutex m_SetMutex;

	#if MM_SDLSOCKET
	#elif MM_WIN32SOCKET
		// Max of 64 for BSD see FD_SETSIZE
		map<SOCKET, pair<sptr<const ISocket>, PacketHandler>> m_Sockets;
		fd_set m_SocketSet;
	#endif
	};
}
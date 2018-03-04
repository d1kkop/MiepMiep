#include "SocketSet.h"
#include <algorithm>
#include <cassert>
using namespace std;


namespace MiepMiep
{
	SocketSet::SocketSet()
	{

	}

	SocketSet::~SocketSet()
	{

	}

	bool SocketSet::addSocket(sptr<ISocket>& sock)
	{
		scoped_lock lk(m_SetMutex);
		#if MM_SDLSOCKET
		#else MM_WIN32SOCKET
			if ( m_Sockets.size() >= FD_SETSIZE ) return false;
			m_Sockets[static_cast<BSDSocket&>(*sock).getSock()] = sock;
		#endif
		return true;
	}

	void SocketSet::removeSocket(sptr<ISocket>& sock)
	{
		scoped_lock lk(m_SetMutex);
		m_Sockets.erase( static_cast<BSDSocket&>(*sock).getSock() );
	}

	EListenOnSocketsResult SocketSet::listenOnSockets(u32 timeoutMs, const std::function<void (sptr<ISocket>&)>& cb, i32* err)
	{
		if ( err ) *err = 0;
		rebuildSocketArrayIfNecessary();

		if ( m_SocketSet.fd_count == 0 )
			return EListenOnSocketsResult::NoSocketsInSet;

		#if MM_SDLSOCKET
		#elif MM_WIN32SOCKET
			// Query sockets for data
			timeval tv;
			tv.tv_sec  = 0;
			tv.tv_usec = timeoutMs*1000;
			i32 res = select( m_SocketSet.fd_count, &m_SocketSet, nullptr, nullptr, &tv );
			if ( res < 0 )
			{
				if ( err ) *err = GetLastError();
				return EListenOnSocketsResult::Error;
			}

			// timed out (no data available)
			if ( res == 0 )
				return EListenOnSocketsResult::TimeoutNoData;

			// at least a single one has data, check which ones and call cb
			scoped_lock lk(m_SetMutex);
			for ( u_int i=0; i<m_SocketSet.fd_count; i++ )
			{
				SOCKET s = m_SocketSet.fd_array[i];
				if ( !FD_ISSET(s, &m_SocketSet) ) continue;
				// map to managed socket
				auto sockIt = m_Sockets.find(s);
				if ( sockIt != m_Sockets.end() )
				{
					sptr<ISocket>& sock = sockIt->second;
					cb( sock ); // has pending read data
				}
			}
		#endif

		return EListenOnSocketsResult::Fine;
	}

	void SocketSet::rebuildSocketArrayIfNecessary()
	{
		scoped_lock lk(m_SetMutex);
		if ( m_Sockets.size() == m_SocketSet.fd_count ) 
			return;
			
		#if MM_SDLSOCKET
		#elif MM_WIN32SOCKET
			FD_ZERO( &m_SocketSet );
			assert( m_Sockets.size() <= FD_SETSIZE );
		#endif
		
		for ( auto& kvp : m_Sockets )
		{
		#if MM_SDLSOCKET
		#elif MM_WIN32SOCKET
			SOCKET s = static_cast<BSDSocket&>(*kvp.second).getSock();
			FD_SET(s, &m_SocketSet);
		#endif
		}
	}

}
#include "SocketSet.h"
#include "Socket.h"
#include "PacketHandler.h"
#include <algorithm>
#include <cassert>
using namespace std;


namespace MiepMiep
{
	SocketSet::SocketSet():
		m_IsDirty(false)
	{
	}

	SocketSet::~SocketSet()
	= default;

	MM_TS bool SocketSet::addSocket(sptr<const ISocket>& sock, const sptr<IPacketHandler>& packetHandler)
	{
		scoped_lock lk(m_SetMutex);
		m_IsDirty = true;
		#if MM_SDLSOCKET
		#else MM_WIN32SOCKET
			if ( m_Sockets.size() >= FD_SETSIZE ) return false;
			m_Sockets[sc<const BSDSocket&>(*sock).getSock()] = make_pair ( sock, packetHandler );
		#endif
		return true;
	}

	MM_TS void SocketSet::removeSocket(const sptr<const ISocket>& sock)
	{
		scoped_lock lk(m_SetMutex);
		m_IsDirty = true;
		#if MM_SDLSOCKET
		#else MM_WIN32SOCKET
			m_Sockets.erase( sc<const BSDSocket&>(*sock).getSock() );
		#endif
	}

	MM_TS bool SocketSet::hasSocket(const sptr<const ISocket>& sock) const
	{
		scoped_lock lk(m_SetMutex);
		#if MM_SDLSOCKET
		#else MM_WIN32SOCKET
			return m_Sockets.count( sc<const BSDSocket&>(*sock).getSock() ) != 0;
		#endif
		return false;
	}

	EListenOnSocketsResult SocketSet::listenOnSockets(u32 timeoutMs, i32* err)
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
			tv.tv_usec = timeoutMs*20*1000; // ~20ms

			// NOTE: select reorders the ready sockets from 0 to end-1, and fd_count is adjusted to the num of ready sockets.
			i32 res = select( 0, &m_SocketSet, nullptr, nullptr, &tv );
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

				// No need to check if 'is_set' because ready sockets are ordered from 0 to end-1.
				assert( FD_ISSET(s, &m_SocketSet) );
				
			//	LOG( "Recv on ... %ull.", s ); 

				// map to managed socket
				auto sockIt = m_Sockets.find(s);
				if ( sockIt != m_Sockets.end() )
				{
					pair<sptr<const ISocket>, sptr<IPacketHandler>>& sockHandler = sockIt->second;
					sockHandler.second->recvFromSocket( *sockHandler.first ); // handle data
				}
			}

		#endif

		return EListenOnSocketsResult::Fine;
	}

	void SocketSet::rebuildSocketArrayIfNecessary()
	{

	#if MM_SDLSOCKET
	#elif MM_WIN32SOCKET

		scoped_lock lk(m_SetMutex);
		// Apparently, we have to rebuild the set each time again.
		m_IsDirty = true;

		if ( m_IsDirty )
		{
			FD_ZERO( &m_SocketSet );
			assert( m_Sockets.size() <= FD_SETSIZE );

			for ( auto& kvp : m_Sockets )
			{
				SOCKET s = sc<const BSDSocket&>(*kvp.second.first).getSock();
				FD_SET(s, &m_SocketSet);
			}

			m_IsDirty = false;
		}
		else
		{
			m_SocketSet.fd_count = (u_int) m_Sockets.size();
		}

	#endif
		
	}

}
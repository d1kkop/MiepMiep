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

	MM_TS bool SocketSet::addSocket(const sptr<const ISocket>& sock, const sptr<IPacketHandler>& packetHandler)
	{
		scoped_lock lk(m_SetMutex);
		m_IsDirty = true;
		#if MM_SDLSOCKET
        #error no implementation
		#else MM_WIN32SOCKET
			if ( m_HighLevelSockets.size() >= FD_SETSIZE ) return false; // Cannot add socket to this set. Create new set.
			m_HighLevelSockets[sc<const BSDSocket&>(*sock).getSock()] = make_pair ( sock, packetHandler );
		#endif
		return true;
	}

	MM_TS void SocketSet::removeSocket(const sptr<const ISocket>& sock)
	{
		scoped_lock lk(m_SetMutex);
		m_IsDirty = true;
		#if MM_SDLSOCKET
        #error no implementation
		#else MM_WIN32SOCKET
			m_HighLevelSockets.erase( sc<const BSDSocket&>(*sock).getSock() );
		#endif
	}

	MM_TS bool SocketSet::hasSocket(const sptr<const ISocket>& sock) const
	{
		scoped_lock lk(m_SetMutex);
		#if MM_SDLSOCKET
        #error no implementation
		#else MM_WIN32SOCKET
			return m_HighLevelSockets.count( sc<const BSDSocket&>(*sock).getSock() ) != 0;
		#endif
		return false;
	}

	EListenOnSocketsResult SocketSet::listenOnSockets(u32 timeoutMs, i32* err)
	{
		if ( err ) *err = 0;
		rebuildSocketArrayIfNecessary();

		if ( m_LowLevelSocketArray.fd_count == 0 )
			return EListenOnSocketsResult::NoSocketsInSet;

		#if MM_SDLSOCKET
        #error no implementation
		#elif MM_WIN32SOCKET

			// Query sockets for data
			timeval tv;
			tv.tv_sec  = 0;
			tv.tv_usec = timeoutMs*1000*MM_SOCK_SELECT_TIMEOUT; // ~20ms

			// NOTE: select reorders the ready sockets from 0 to end-1, and fd_count is adjusted to the num of ready sockets.
			i32 res = select( 0, &m_LowLevelSocketArray, nullptr, nullptr, &tv );
			if ( res < 0 )
			{
				if ( err ) *err = GetLastError();
				return EListenOnSocketsResult::Error;
			}

			// timed out (no data available)
			if ( res == 0 )
			{
				return EListenOnSocketsResult::TimeoutNoData;
			}

			// at least a single one has data, check which ones and call cb
			for ( i32 i=0; i<res; i++ )
			{
				SOCKET s = m_LowLevelSocketArray.fd_array[i];

				// Because only checking 'read' sockets, s must be SET.
				assert( FD_ISSET(s, &m_LowLevelSocketArray) );
				
			//	LOG( "Recv on ... %ull.", s ); 

				// Only use lock for obtaining socket, then release as there is no need to keep lock.
				pair<sptr<const ISocket>, sptr<IPacketHandler>> sockHandlerPair;
				bool validPair = false;
				{
					scoped_lock lk( m_SetMutex );
					auto sockIt = m_HighLevelSockets.find( s );
					if ( sockIt != m_HighLevelSockets.end() )
					{
						sockHandlerPair = sockIt->second;
						validPair = true;
					}
				}
				
				// Start receiving the packet.
				if ( validPair )
				{
					auto& receptionSock = sockHandlerPair.first;
					auto& packetHandler = sockHandlerPair.second;
					packetHandler->recvFromSocket( *receptionSock ); // handle data
				}
			}

		#endif

		return EListenOnSocketsResult::Fine;
	}

	void SocketSet::rebuildSocketArrayIfNecessary()
	{

	#if MM_SDLSOCKET
        #error no implementation
	#elif MM_WIN32SOCKET

		scoped_lock lk(m_SetMutex);
		// Apparently, we have to rebuild the set each time again.
		m_IsDirty = true;

		if ( m_IsDirty )
		{
			FD_ZERO( &m_LowLevelSocketArray );
			assert( m_HighLevelSockets.size() <= FD_SETSIZE );

			for ( auto& kvp : m_HighLevelSockets )
			{
				SOCKET s = sc<const BSDSocket&>(*kvp.second.first).getSock();
				FD_SET(s, &m_LowLevelSocketArray);
			}

			m_IsDirty = false;
		}
		else
		{
			m_LowLevelSocketArray.fd_count = (u_int) m_HighLevelSockets.size();
		}

	#endif
		
	}

}
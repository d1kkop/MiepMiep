#include "Socket.h"
#include "Platform.h"
#include "Endpoint.h"

#include <cassert>


namespace MiepMiep
{
	ISocket::ISocket():
		m_Open(false),
		m_Bound(false),
		m_IpProto(IPProto::Ipv4)
	{
	}

	sptr<ISocket> ISocket::create()
	{
		if ( 0 == Platform::initialize() )
		{
		#if MM_SDLSOCKET
			return reserve_sp<SDLSocket>(MM_FL);
		#elif MM_WIN32SOCKET
			return reserve_sp<BSDSocket>(MM_FL);
		#endif
		}
		return nullptr;
	}

	ISocket::~ISocket()
	{
		Platform::shutdown();
	}

	bool ISocket::operator==(const ISocket& right) const
	{
		return this->equal(right);
	}

#if MM_SDLSOCKET

	// ------------ SDLSocket ------------------------------------------------------------------------------------

	SDLSocket::SDLSocket():
		m_Socket(nullptr),
		m_SocketSet(nullptr)
	{
		m_Blocking = true;
		m_SocketSet = SDLNet_AllocSocketSet(1);
		if (!m_SocketSet)
		{
			m_LastError = SocketError::CannotCreateSet;
		}
	}

	SDLSocket::~SDLSocket()
	{
		assert (!m_Open);
		close();
		SDLNet_FreeSocketSet(m_SocketSet);
	}

	bool SDLSocket::open(IPProto ipProto, bool reuseAddr)
	{
		if (m_LastError!=SocketError::Succes) return false;
		m_Open = true;
		return m_Open;
	}

	bool SDLSocket::bind(u16_t port)
	{
		if (m_LastError!=SocketError::Succes) return false;
		if (!m_Open) 
		{
			m_LastError = SocketError::NotOpened;
			return false;
		}
		if (m_Bound) return true;
		assert(m_SocketSet && !m_Socket);
		m_Socket = SDLNet_UDP_Open(port);
		if (!m_Socket) 
		{
			// binding multiple times on same port already returns -1 (as it turns out)
			m_LastError = SocketError::PortAlreadyInUse;
			//m_LastError = SocketError::CannotOpen;
			return false;
		}
		IPaddress* ip =  SDLNet_UDP_GetPeerAddress(m_Socket, -1);
		if (!ip)
		{
			m_LastError = SocketError::CannotResolveLocalAddress;
			return false;
		}
		int channel = SDLNet_UDP_Bind(m_Socket, 0, ip);
		if (channel != 0)
		{
			m_LastError = SocketError::PortAlreadyInUse;
			return false;
		}
		if (-1 == SDLNet_UDP_AddSocket(m_SocketSet, m_Socket))
		{
			m_LastError = SocketError::CannotAddToSet;
			return false;
		}
		m_Bound = true;
		return m_Bound;
	}

	bool SDLSocket::close()
	{
		m_Open = false;
		if (m_Socket != nullptr)
		{
			if (m_Bound)
			{
				SDLNet_UDP_Unbind(m_Socket, 0);
				SDLNet_UDP_DelSocket(m_SocketSet, m_Socket); // ignore errors here as is closing routine
			}
			m_Bound = false;
			SDLNet_UDP_Close(m_Socket);
			m_Socket = nullptr;
		}
		// clean it all
		m_LastError = SocketError::Succes;
		return true;
	}

	ESendResult SDLSocket::send( const struct EndPoint& endPoint, const i8_t* data, i32_t len ) const
	{
		if ( m_Socket == nullptr )
			return ESendResult::SocketClosed;

		IPaddress dstIp;
		dstIp.host = endPoint.getIpv4NetworkOrder();
		dstIp.port = endPoint.getPortNetworkOrder();

		UDPpacket pack;
		pack.len     = len;
		pack.maxlen  = len;
		pack.address = dstIp;
		pack.data    = (Uint8*)data;
//		Platform::memCpy( pack.data, len, data, len );

		if ( 1 != SDLNet_UDP_Send( m_Socket, -1, &pack ) )
		{
			m_LastError = SocketError::SendFailure;
			Platform::log("SDL send udp packet error %s", SDLNet_GetError());
			return ESendResult::Error;
		}

//		SDLNet_FreePacket(pack);
		return ESendResult::Succes;
	}

	ERecvResult SDLSocket::recv( i8_t* buff, i32_t& rawSize, struct EndPoint& endPoint ) const
	{
		if (!m_Socket || !m_SocketSet)
			return ERecvResult::SocketClosed;

		i32_t res = SDLNet_CheckSockets( m_SocketSet, 100 );
		if (!m_Open || !m_Socket) // if closing, ignore error
			return ERecvResult::SocketClosed;
		if ( res == -1 )
			return ERecvResult::Error;
		if ( res == 0 || !SDLNet_SocketReady(m_Socket) )
			return ERecvResult::NoData;

		UDPpacket packet = { 0 };
		packet.data = (Uint8 *) buff;
		packet.len  = rawSize;
		packet.maxlen = rawSize;
		i32_t numPackets = SDLNet_UDP_Recv( m_Socket, &packet );
		
		if (!m_Open)
		{
			return ERecvResult::SocketClosed;
		}

		if ( numPackets == 0 )
		{
			return ERecvResult::NoData;
		}

		if ( -1 == numPackets )
		{
			Platform::log("SDL recv error %s.:", SDLNet_GetError());
			m_LastError = SocketError::RecvFailure;
			return ERecvResult::Error;
		}

		rawSize = packet.len;
		endPoint.setIpAndPortFromNetworkOrder( packet.address.host, packet.address.port );
//		Platform::memCpy( buff, rawSize, packet.data, packet.len );

		return ERecvResult::Succes;
	}

#elif MM_WIN32SOCKET

	// --------------- BSDWin32 Socket ------------------------------------------------------------------------------------

	BSDSocket::BSDSocket():
		m_Socket(INVALID_SOCKET)
	{
		m_Blocking = true;
	}

	bool BSDSocket::open(IPProto ipv, bool reuseAddr, i32* err)
	{
		if ( err ) *err = 0;

		// open can only be set if previous open was succesful
		if ( m_Open && m_Socket != INVALID_SOCKET )
		{
			return true;
		}
		// reset open state if was invalid socket
		m_Open = false;
		m_IpProto = ipv;

		m_Socket = socket( (ipv == IPProto::Ipv4 ? AF_INET : AF_INET6), SOCK_DGRAM, IPPROTO_UDP) ;
		if (m_Socket == INVALID_SOCKET)
		{
			#if MM_PLATFORM_WINDOWS
				if ( err ) *err = GetLastError();
			#endif
			return false;
		}

		BOOL bReuseAddr = reuseAddr ? TRUE : FALSE;
		if ( SOCKET_ERROR == setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuseAddr, sizeof(BOOL)) )
		{
			#if MM_PLATFORM_WINDOWS
				if ( err ) *err = GetLastError();
			#endif
			return false;
		}

		m_Open = true;
		return true;
	}

	bool BSDSocket::bind(u16 port, i32* err)
	{
		if ( err ) *err = 0;

		if ( m_Socket == INVALID_SOCKET )
			return false;

		// bound can only be set if previous bound was succesful
		if ( m_Bound )
			return true;

		addrinfo hints;
		addrinfo *addrInfo = nullptr;

		memset(&hints, 0, sizeof(hints));

		hints.ai_family = m_IpProto == IPProto::Ipv4 ? PF_INET : PF_INET6;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		hints.ai_flags = AI_PASSIVE; // <-- means that we intend to bind the socket

		char portBuff[32];
		Platform::formatPrint(portBuff, 32, "%d", port);
		if (0 != getaddrinfo(nullptr, portBuff, &hints, &addrInfo))
		{
			#if MM_PLATFORM_WINDOWS
				if ( err ) *err = GetLastError();
			#endif
			freeaddrinfo(addrInfo);
			return false;
		}

		// try binding on the found host addresses
		for (addrinfo* inf = addrInfo; inf != nullptr; inf = inf->ai_next)
		{
			if (0 == ::bind(m_Socket, inf->ai_addr, (i32)inf->ai_addrlen))
			{
				freeaddrinfo(addrInfo);
				// succesful bind
				m_Bound = true;
				return true;
			}
		}

		#if MM_PLATFORM_WINDOWS
			if ( err ) *err = GetLastError();
		#endif
		return false;
	}

	void BSDSocket::close()
	{
		if ( m_Socket != INVALID_SOCKET )
		{
			#if MM_PLATFORM_WINDOWS
				closesocket( m_Socket );
			#else	
				close( m_Socket );
			#endif
			m_Socket = INVALID_SOCKET;
		}
		m_Open  = false;
		m_Bound = false;
	}

	bool BSDSocket::equal(const ISocket& other) const
	{
		const BSDSocket& b = static_cast<const BSDSocket&>(other);
		return (this->m_Socket == b.m_Socket) && (this->m_Socket != INVALID_SOCKET);
	}

	ESendResult BSDSocket::send(const Endpoint& endPoint, const byte* data, u32 len, i32* err) const
	{
		if ( err ) *err = 0;

		if ( m_Socket == INVALID_SOCKET )
			return ESendResult::SocketClosed;

		const byte* addr	= endPoint.getLowLevelAddr();
		u32 addrSize		= endPoint.getLowLevelAddrSize();

		if ( SOCKET_ERROR == sendto( m_Socket, ( const char* ) data, len, 0, (const sockaddr*)addr, addrSize ) )
		{
			#if MM_PLATFORM_WINDOWS
				if ( err ) *err = GetLastError();
			#endif
			return ESendResult::Error;
		}

		return ESendResult::Succes;
	}

	ERecvResult BSDSocket::recv(byte* buff, u32& rawSize, Endpoint& endPoint, i32* err) const
	{
		if (err) *err = 0;

		if ( m_Socket == INVALID_SOCKET )
			return ERecvResult::SocketClosed;

		i32 addrSize = (i32) endPoint.getLowLevelAddrSize();
		rawSize = recvfrom(m_Socket, (char*) buff, rawSize, 0, (sockaddr*)endPoint.getLowLevelAddr(), (int*) &addrSize);

		if ( rawSize < 0 )
		{
			#if MM_PLATFORM_WINDOWS
				if ( err ) *err = GetLastError();
			#endif
			return ERecvResult::Error;
		}

		return ERecvResult::Succes;
	}

	sptr<ISocket> BSDSocket::to_ptr()
	{
		return static_pointer_cast<ISocket>( static_cast<BSDSocket&>(*this).ptr<BSDSocket>() );
	}

	sptr<const ISocket> BSDSocket::to_ptr() const
	{
		return static_pointer_cast<const ISocket>( static_cast<const BSDSocket&>(*this).ptr<const BSDSocket>() );
	}

#endif
}
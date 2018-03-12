#include "Endpoint.h"
#include "Util.h"
#include <cassert>
using namespace std;


namespace MiepMiep
{
	// --- IEndpoint ---------------------------------------------------------------------------------------

	sptr<IEndpoint> IEndpoint::resolve( const string& name, u16 port, i32* errOut )
	{
		sptr<Endpoint> etp = Endpoint::resolve( name, port, errOut );
		return etp;
	}

	sptr<IEndpoint> IEndpoint::fromIpAndPort( const string& ipAndPort, i32* errOut )
	{
		sptr<Endpoint> etp = Endpoint::fromIpAndPort( ipAndPort );
		return etp;
	}

	sptr<IEndpoint> Endpoint::getCopy() const
	{
		sptr<Endpoint> etp = reserve_sp<Endpoint>(MM_FL);
		memcpy((void*)etp->getLowLevelAddr(), getLowLevelAddr(), getLowLevelAddrSize());
		return static_pointer_cast<IEndpoint>( etp );
	}

	MM_TS sptr<Endpoint> Endpoint::getCopyDerived() const
	{
		return static_pointer_cast<Endpoint>( getCopy() );
	}

	bool IEndpoint::operator==(const IEndpoint& other) const
	{
		const Endpoint& a = static_cast<const Endpoint&>(*this);
		const Endpoint& b = static_cast<const Endpoint&>(other);
		return a==b;
	}

	bool IEndpoint::stl_less::operator()(const sptr<const IEndpoint>& left, const sptr<const IEndpoint>& right) const
	{
		const Endpoint& a = static_cast<const Endpoint&>(*left);
		const Endpoint& b = static_cast<const Endpoint&>(*right);
		return Endpoint::compareLess(a, b) < 0;
	}

	sptr<IEndpoint> IEndpoint::to_ptr()
	{
		return static_pointer_cast<IEndpoint>( static_cast<Endpoint&>(*this).ptr<Endpoint>() );
	}

	sptr<const IEndpoint> IEndpoint::to_ptr() const
	{
		return static_pointer_cast<const IEndpoint>( static_cast<const Endpoint&>(*this).ptr<const Endpoint>() );
	}


	// --- Endpoint ---------------------------------------------------------------------------------------

	sptr<Endpoint> Endpoint::resolve( const string& name, u16 port, i32* errorOut )
	{
		if ( errorOut ) *errorOut = 0;

		#if MM_SDLSOCKET
			// transforms port into network order
			if ( 0 == SDLNet_ResolveHost( &m_IpAddress, name.c_str(), port ) )
			{
				return nullptr;
			}
		#elif MM_WIN32SOCKET
			addrinfo hints;
			addrinfo *addrInfo = nullptr;

			memset(&hints, 0, sizeof(hints));

			hints.ai_family   = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_protocol = IPPROTO_UDP;
			// hints.ai_flags = AI_PASSIVE; <-- use this for binding, otherwise connecting

			char ipBuff[128];
			char portBuff[32];
			// TODO check thread safety
			Platform::formatPrint(ipBuff, 128, "%s", name.c_str());
			Platform::formatPrint(portBuff, 32, "%d", port);

			i32 err = getaddrinfo(ipBuff, portBuff, &hints, &addrInfo);
			if (err != 0)
			{
				if ( errorOut ) *errorOut = err;
				freeaddrinfo(addrInfo);
				return nullptr;
			}

			// try binding on the found host addresses
			for (addrinfo* inf = addrInfo; inf != nullptr; inf = inf->ai_next)
			{
				sptr<Endpoint> etp = reserve_sp<Endpoint>(MM_FL);
				memcpy( &etp->m_SockAddr, inf->ai_addr, inf->ai_addrlen );
				freeaddrinfo(addrInfo);
				return etp;
			}

			freeaddrinfo(addrInfo);
			return nullptr;
		#endif
		return nullptr;
	}

	sptr<Endpoint> Endpoint::fromIpAndPort(const string& ipAndPort, i32* errOut)
	{
		if ( errOut ) *errOut = -1;

		for ( u64 i=ipAndPort.length(); i!=0; i-- )
		{
			if ( ipAndPort[i] == ':' )
			{
				i32 l = (i32)( ipAndPort.length() - (i+1) );
				if ( l > 0 )
				{
					u16 port = (u16) atoi(ipAndPort.substr(i+1, l).c_str());
					return resolve( ipAndPort.substr(0, i), port, errOut );
				}
			}
		}
		return nullptr;
	}

	string Endpoint::toIpAndPort() const
	{
		#if MM_SDLSOCKET
			IPaddress iph;
			iph.host = Util::ntohl(m_IpAddress.host);
			iph.port = Util::ntohs(m_IpAddress.port);
			byte* ip = (byte*)&iph.host;
			//std::string ip = SDLNet_ResolveIP(&iph); // expects in host form <-- is superrrr !!-SLOW-!! function
			char buff[1024];
			#if MM_LIL_ENDIAN
				Platform::formatPrint(buff, 1024, "%d.%d.%d.%d:%d", ip[3], ip[2], ip[1], ip[0], getPortHostOrder());
			#elif MM_BIG_ENDIAN
				Platform::formatPrint(buff, 1024, "%d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], getPortHostOrder());
			#endif
			return string(buff);
		#endif

		#if MM_WIN32SOCKET
			char ipBuff[128] = { 0 };
			inet_ntop(m_SockAddr.si_family, (PVOID)&m_SockAddr.Ipv4, ipBuff, 128);
			return string(ipBuff) + ":" + to_string(getPortHostOrder());
		#endif

		return "";
	}

	bool Endpoint::operator==(const Endpoint& other) const
	{
		return compareLess(*this, other) == 0;
	}

	u16 Endpoint::getPortHostOrder() const
	{
		return Util::ntohs(getPortNetworkOrder());
	}

	u16 Endpoint::getPortNetworkOrder() const
	{
		#if MM_SDLSOCKET
			return m_IpAddress.port;
		#elif MM_WIN32SOCKET
			return m_SockAddr.Ipv4.sin_port;
		#endif
		return -1;
	}

	u32 Endpoint::getIpv4HostOrder() const
	{
		return Util::ntohl(getIpv4NetworkOrder());
	}

	u32 Endpoint::getIpv4NetworkOrder() const
	{
		#if MM_SDLSOCKET
			return m_IpAddress.host;
		#elif MM_WIN32SOCKET
			return m_SockAddr.Ipv4.sin_addr.S_un.S_addr;
		#endif
		assert(0);
		return -1;
	}

	const void* Endpoint::getLowLevelAddr() const
	{
		#if MM_SDLSOCKET
			return &m_IpAddress;
		#elif MM_WIN32SOCKET
			return &m_SockAddr;
		#endif
		assert(0);
		return nullptr;
	}

	u32 Endpoint::getLowLevelAddrSize() const
	{
		#if MM_SDLSOCKET
			return sizeof(m_IpAddress);
		#elif MM_WIN32SOCKET
			return sizeof(m_SockAddr);
		#endif
		assert(0);
		return -1;
	}

	void Endpoint::setIpAndPortFromNetworkOrder(u32 ip, u16 port)
	{
		#if MM_SDLSOCKET
			m_IpAddress.host = ip;
			m_IpAddress.port = port;
		#elif MM_WIN32SOCKET
			memset(&m_SockAddr, 0, sizeof(m_SockAddr));
			m_SockAddr.Ipv4.sin_port = port;
			m_SockAddr.Ipv4.sin_addr.S_un.S_addr = ip;
			m_SockAddr.si_family = AF_INET;
			m_SockAddr.Ipv4.sin_family = AF_INET;
		#endif
	}

	void Endpoint::setIpAndPortFromHostOrder(u32 ip, u16 port)
	{
		setIpAndPortFromNetworkOrder(Util::htonl(ip), Util::htons(port));
	}

	i32 Endpoint::compareLess(const Endpoint& a, const Endpoint& b)
	{
		return ::memcmp( a.getLowLevelAddr(), b.getLowLevelAddr(), a.getLowLevelAddrSize() ) ;
	}

}
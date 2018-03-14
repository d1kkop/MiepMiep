#include "Endpoint.h"
#include "Util.h"
#include <cassert>
using namespace std;


namespace MiepMiep
{
	// --- IEndpoint ---------------------------------------------------------------------------------------

	sptr<IEndpoint> IEndpoint::createEmpty()
	{
		return Endpoint::createEmpty();
	}

	sptr<IEndpoint> IEndpoint::resolve( const string& name, u16 port, i32* errOut )
	{
		return Endpoint::resolve( name, port, errOut );
	}

	sptr<IEndpoint> IEndpoint::fromIpAndPort( const string& ipAndPort, i32* errOut )
	{
		return Endpoint::fromIpAndPort( ipAndPort );
	}

	sptr<IEndpoint> Endpoint::getCopy() const
	{
		return getCopyDerived();
	}

	MM_TS sptr<Endpoint> Endpoint::getCopyDerived() const
	{
		sptr<Endpoint> etp = reserve_sp<Endpoint>(MM_FL);
		Platform::copy(etp->getLowLevelAddr(), getLowLevelAddr(), getLowLevelAddrSize());
		return etp;
	}

	MM_TS bool Endpoint::write(class BinSerializer& bs) const
	{
		if (!bs.write(getLowLevelAddr(), getLowLevelAddrSize())) return false;
		return true;
	}

	MM_TS bool Endpoint::read(class BinSerializer& bs)
	{
		if (!bs.read(getLowLevelAddr(), getLowLevelAddrSize() ) ) return false;
		return true;
	}

	bool IEndpoint::operator==(const IEndpoint& other) const
	{
		const Endpoint& a = sc<const Endpoint&>(*this);
		const Endpoint& b = sc<const Endpoint&>(other);
		return a==b;
	}

	bool IEndpoint_less::operator()( const IEndpoint& left, const IEndpoint& right ) const
	{
		const Endpoint& a = sc<const Endpoint&>(left);
		const Endpoint& b = sc<const Endpoint&>(right);
		return Endpoint::compareLess(a, b) < 0;
	}

	bool IEndpoint_less::operator()( const sptr<const IEndpoint>& left, const sptr<const IEndpoint>& right ) const
	{
		const Endpoint& a = sc<const Endpoint&>(*left);
		const Endpoint& b = sc<const Endpoint&>(*right);
		return Endpoint::compareLess(a, b) < 0;
	}

	sptr<IEndpoint> IEndpoint::to_ptr()
	{
		return sc<Endpoint&>(*this).ptr<Endpoint>();
	}

	sptr<IEndpoint> IEndpoint::to_ptr_nc() const
	{
		return sc<Endpoint&>(const_cast<IEndpoint&>(*this)).ptr<Endpoint>();
	}

	sptr<const IEndpoint> IEndpoint::to_ptr() const
	{
		return sc<const Endpoint&>(*this).ptr<const Endpoint>();
	}


	// --- Endpoint ---------------------------------------------------------------------------------------

	MM_TS sptr<Endpoint> Endpoint::createEmpty()
	{
		return reserve_sp<Endpoint>(MM_FL);
	}

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
				Platform::memCpy( &etp->m_SockAddr, sizeof(etp->m_SockAddr), inf->ai_addr, inf->ai_addrlen );
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

	const char* Endpoint::toIpAndPort() const
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
			static thread_local char ipBuff[64] = { 0 };
			if ( m_SockAddr.si_family == AF_INET )
			{
				inet_ntop(m_SockAddr.si_family, (PVOID)&m_SockAddr.Ipv4.sin_addr.s_addr, ipBuff, sizeof(ipBuff));
			}
			else
			{
				inet_ntop(m_SockAddr.si_family, (PVOID)&m_SockAddr.Ipv6.sin6_addr, ipBuff, sizeof(ipBuff));
			}
			static thread_local char ipAndPort[64] = { 0 };
			Platform::formatPrint(ipAndPort, sizeof(ipAndPort), "%s:%d", ipBuff, getPortHostOrder());
			return ipAndPort;
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
			// How I understand it, it doesnt matter which one is picked, its all in a union and equally aligned. So pick one.
			return m_SockAddr.Ipv4.sin_port;
		#endif
		return -1;
	}

	byte* Endpoint::getLowLevelAddr()
	{
		#if MM_SDLSOCKET
			return &m_IpAddress;
		#elif MM_WIN32SOCKET
			return rc<byte*>(&m_SockAddr);
		#endif
		assert(0);
		return nullptr;
	}

	const byte* Endpoint::getLowLevelAddr() const
	{
		return const_cast<Endpoint*>(this)->getLowLevelAddr();
		//return const_cast<byte*>( getLowLevelAddr() );
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

	i32 Endpoint::compareLess(const Endpoint& a, const Endpoint& b)
	{
		return ::memcmp( a.getLowLevelAddr(), b.getLowLevelAddr(), a.getLowLevelAddrSize() ) ;
	}

	void Endpoint::setPortFromNetworkOrder(u16 port)
	{
	#if MM_SDLSOCKET
		m_IpAddress.port = port;
		return;
	#elif MM_WIN32SOCKET
		m_SockAddr.Ipv4.sin_port = port;
		return;
	#endif
		assert(0);
		return;
	}

}
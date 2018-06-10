#include "Endpoint.h"
#include "Socket.h"
#include "Util.h"
#include <cassert>
#include <algorithm>
using namespace std;


namespace MiepMiep
{
	// --- IAddress ---------------------------------------------------------------------------------------

	sptr<IAddress> IAddress::createEmpty()
	{
		return Endpoint::createEmpty();
	}

	sptr<IAddress> IAddress::resolve( const string& name, u16 port, i32* errOut )
	{
		return Endpoint::resolve( name, port, errOut );
	}

	sptr<IAddress> IAddress::fromIpAndPort( const string& ipAndPort, i32* errOut )
	{
		return Endpoint::fromIpAndPort( ipAndPort );
	}

	MM_TS bool IAddress::operator<(const IAddress& other) const
	{
		const Endpoint& a = sc<const Endpoint&>(*this);
		const Endpoint& b = sc<const Endpoint&>(other);
		return a < b;
	}

	bool IAddress::operator==(const IAddress& other) const
	{
		const Endpoint& a = sc<const Endpoint&>(*this);
		const Endpoint& b = sc<const Endpoint&>(other);
		return a==b;
	}

	MM_TSC sptr<IAddress> IAddress::to_ptr()
	{
		return sc<Endpoint&>(*this).ptr<Endpoint>();
	}

	MM_TSC sptr<const IAddress> IAddress::to_ptr() const
	{
		return sc<const Endpoint&>(*this).ptr<const Endpoint>();
	}


	// --- AddressList ---------------------------------------------------------------------------------------

	sptr<AddressList> AddressList::create()
	{
		sptr<AddressList> addrList = reserve_sp<AddressList>( MM_FL );
		return addrList;
	}

	MM_TS void AddressList::addAddress( const IAddress& addr, const MetaData& metaData )
	{
		sptr<AddrMetaDataPair> adp = reserve_sp<AddrMetaDataPair>( MM_FL );
		adp->m_Addr = addr.to_ptr();
		adp->m_Md = metaData;
		scoped_lock lk( m_AddressMutex );
		m_Addresses.emplace_back( adp );
	}

	MM_TS void AddressList::removeAddress( const IAddress* addr )
	{
		if ( !addr ) return;
		scoped_lock lk( m_AddressMutex );
		m_Addresses.erase( std::find_if( m_Addresses.begin(), m_Addresses.end(), [&] ( auto& adp ) { return *adp->m_Addr==*addr; } ) );
	}

	MM_TS bool AddressList::readOrWrite( BinSerializer& bs, bool _write )
	{
		scoped_lock lk( m_AddressMutex );
		if ( _write )
		{
			for ( auto& addr : m_Addresses )
			{
				__CHECKEDB( bs.write( addr->m_Addr ) );
				__CHECKEDB( bs.write( addr->m_Md ) );
			}
		}
		else
		{
			while ( bs.getRead() != bs.getWrite() )
			{
				sptr<AddrMetaDataPair> adp = reserve_sp<AddrMetaDataPair>( MM_FL );
				sptr<IAddress> addr;
				__CHECKEDB( bs.read( addr ) );
				__CHECKEDB( bs.read( adp->m_Md ) );
				adp->m_Addr = addr;
				m_Addresses.emplace_back( adp );
			}
		}
		return true;
	}

	MM_TS MetaData AddressList::metaData( u32 idx ) const
	{
		scoped_lock lk( m_AddressMutex );
		if ( idx < m_Addresses.size() )
		{
			return m_Addresses[idx]->m_Md;
		}
		return MetaData();
	}

	MM_TS sptr<const IAddress> AddressList::address( u32 idx ) const
	{
		scoped_lock lk( m_AddressMutex );
		if ( idx < m_Addresses.size() )
		{
			return m_Addresses[idx]->m_Addr;
		}
		return nullptr;
	}

	MM_TS u32 AddressList::count() const
	{
		scoped_lock lk( m_AddressMutex );
		return (u32)m_Addresses.size();
	}

	// --- Endpoint ---------------------------------------------------------------------------------------

	Endpoint::Endpoint()
	{
	#if MM_WIN32SOCKET
		memset(&m_SockAddr, 0, sizeof(m_SockAddr));
	#endif
	}

	MM_TS sptr<Endpoint> Endpoint::createEmpty()
	{
		return reserve_sp<Endpoint>(MM_FL);
	}

	MM_TS sptr<Endpoint> Endpoint::createSource(const ISocket& sock, i32* errOut)
	{
		if ( errOut ) *errOut = 0;

	#if MM_SDLSOCKET
		// transforms port into network order
		if ( 0 == SDLNet_ResolveHost( &m_IpAddress, name.c_str(), port ) )
		{
			return nullptr;
		}
	#elif MM_WIN32SOCKET

		SOCKADDR_INET addr;
		int addrNameLen = sizeof(addr);
		const BSDSocket& bsdSock = sc<const BSDSocket&>(sock);
		if ( 0 != getsockname(bsdSock.getSock(), (SOCKADDR*)&addr, &addrNameLen) )
		{
			if ( errOut ) *errOut = WSAGetLastError();
			return nullptr;
		}
		sptr<Endpoint> etp = reserve_sp<Endpoint>(MM_FL);
	#if _DEBUG
		etp->m_HostPort = Util::ntohs( addr.Ipv4.sin_port );
	#endif
		Platform::memCpy( &etp->m_SockAddr, sizeof(etp->m_SockAddr), &addr, addrNameLen );
		return etp;

	#endif

		assert(false);
		return nullptr;
	}

	MM_TS sptr<Endpoint> Endpoint::resolve(const string& name, u16 port, i32* errorOut)
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
			#if _DEBUG
				etp->m_HostPort = port;
			#endif
				return etp;
			}

			freeaddrinfo(addrInfo);
			return nullptr;
		#endif

		assert(false);
		return nullptr;
	}

	MM_TS sptr<Endpoint> Endpoint::fromIpAndPort(const string& ipAndPort, i32* errOut)
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

	MM_TSC const char* Endpoint::toIpAndPort() const
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

	MM_TSC sptr<IAddress> Endpoint::getCopy() const
	{
		return getCopyDerived();
	}

	MM_TSC sptr<Endpoint> Endpoint::getCopyDerived() const
	{
		sptr<Endpoint> etp = reserve_sp<Endpoint>(MM_FL);
		Platform::copy(etp->getLowLevelAddr(), getLowLevelAddr(), getLowLevelAddrSize());
	#if _DEBUG
		etp->m_HostPort = m_HostPort;
	#endif
		return etp;
	}

	MM_TSC u16 Endpoint::port() const
	{
		return getPortHostOrder();
	}

	bool Endpoint::write( class BinSerializer& bs ) const
	{
		if (!bs.write(getLowLevelAddr(), getLowLevelAddrSize())) return false;
		return true;
	}

	bool Endpoint::read(class BinSerializer& bs)
	{
		if (!bs.read(getLowLevelAddr(), getLowLevelAddrSize() ) ) return false;
		return true;
	}

	MM_TSC bool Endpoint::operator<(const Endpoint& other) const
	{
		auto as = getLowLevelAddrSize();
		auto bs = other.getLowLevelAddrSize();
		if ( as < bs ) return true;
		if ( as > bs ) return false;
		return ::memcmp(getLowLevelAddr(), other.getLowLevelAddr(), getLowLevelAddrSize()) < 0;
	}

	MM_TSC bool Endpoint::operator==(const Endpoint& other) const
	{
		auto as = getLowLevelAddrSize();
		auto bs = other.getLowLevelAddrSize();
		if ( as != bs ) return false;
		return ::memcmp(getLowLevelAddr(), other.getLowLevelAddr(), getLowLevelAddrSize()) == 0;
	}

	MM_TSC u16 Endpoint::getPortHostOrder() const
	{
		return Util::ntohs(getPortNetworkOrder());
	}

	MM_TSC u16 Endpoint::getPortNetworkOrder() const
	{
		#if MM_SDLSOCKET
			return m_IpAddress.port;
		#elif MM_WIN32SOCKET
			// How I understand it, it doesnt matter which one is picked, its all in a union and equally aligned. So pick one.
			return m_SockAddr.Ipv4.sin_port;
		#endif
		return -1;
	}

	MM_TSC byte* Endpoint::getLowLevelAddr()
	{
		#if MM_SDLSOCKET
			return &m_IpAddress;
		#elif MM_WIN32SOCKET
			return rc<byte*>(&m_SockAddr);
		#endif
		assert(0);
		return nullptr;
	}

	MM_TSC bool Endpoint::isZero() const
	{
		Endpoint temp;
		return *this == temp;
	}

	MM_TSC const byte* Endpoint::getLowLevelAddr() const
	{
		return const_cast<Endpoint*>(this)->getLowLevelAddr();
		//return const_cast<byte*>( getLowLevelAddr() );
	}

	MM_TSC u32 Endpoint::getLowLevelAddrSize() const
	{
		#if MM_SDLSOCKET
			return sizeof(m_IpAddress);
		#elif MM_WIN32SOCKET
			return sizeof(m_SockAddr);
		#endif
		assert(0);
		return -1;
	}

}
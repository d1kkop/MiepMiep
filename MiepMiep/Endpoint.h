#pragma once

#include "Platform.h"
#include "Memory.h"
#include "MiepMiep.h"
using namespace std;


namespace MiepMiep
{
	class ISocket;

	class Endpoint: public IAddress, public ITraceable
	{
	public:
		MM_TS static sptr<Endpoint> createEmpty();
		MM_TS static sptr<Endpoint> createSource( const ISocket& sock, i32* errOut = nullptr );
		MM_TS static sptr<Endpoint> resolve( const string& name, u16 port, i32* errOut=nullptr );
		MM_TS static sptr<Endpoint> fromIpAndPort( const string& ipAndPort, i32* errOut=nullptr );

		// IAddress
		const char* toIpAndPort() const override;
		sptr<IAddress> getCopy() const override;
		sptr<Endpoint> getCopyDerived() const;
		u16 port() const override;

		bool write(class BinSerializer& bs) const override;
		bool read(class BinSerializer& bs) override;
		
		bool operator< ( const Endpoint& other) const;
		bool operator==( const Endpoint& other ) const;
		bool operator==( const sptr<Endpoint>& other )	const		{ return *this == *other; }
		bool operator!=( const Endpoint& other ) const				{ return !(*this == other); }
		bool operator!=( const sptr<Endpoint>& other )				{ return !(*this == *other); }	

		u16 getPortHostOrder() const;
		u16 getPortNetworkOrder() const;

		// Either ipv4 or ipv6
		byte* getLowLevelAddr();
		const byte* getLowLevelAddr() const;
		u32 getLowLevelAddrSize() const;

	private:
		#if MM_SDLSOCKET
			IPaddress m_IpAddress;
		#elif  MM_WIN32SOCKET
			SOCKADDR_INET m_SockAddr;
		#endif
	};
}
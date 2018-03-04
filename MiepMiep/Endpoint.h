#pragma once

#include "Platform.h"
#include "Memory.h"
#include "MiepMiep.h"
using namespace std;


namespace MiepMiep
{
	class Endpoint: public IEndpoint, public ITraceable
	{
	public:
		static sptr<Endpoint> resolve( const string& name, u16 port, i32* errOut=nullptr );
		static sptr<Endpoint> fromIpAndPort( const string& ipAndPort, i32* errOut=nullptr );

		// IEndpoint
		string toIpAndPort() const override;
		sptr<IEndpoint> getCopy() const override;
		
		bool operator==( const Endpoint& other ) const;
		bool operator==( const sptr<Endpoint>& other )	const		{ return *this == *other; }
		bool operator!=( const Endpoint& other ) const				{ return !(*this == other); }
		bool operator!=( const sptr<Endpoint>& other )				{ return !(*this == *other); }	

		u16 getPortHostOrder() const;
		u16 getPortNetworkOrder() const;
		u32 getIpv4HostOrder() const;
		u32 getIpv4NetworkOrder() const;

		// Returns ptr to actual host data for compatability with BSD
		const void* getLowLevelAddr() const;

		// Returns size of lowLevel ptr
		u32 getLowLevelAddrSize() const;

		void setIpAndPortFromNetworkOrder( u32 ip, u16 port ); // network order is big endian
		void setIpAndPortFromHostOrder( u32 ip, u16 port );

		// STL compare less
		static i32 compareLess( const Endpoint& a, const Endpoint& b );

	private:
		#if MM_SDLSOCKET
			IPaddress m_IpAddress;
		#elif  MM_WIN32SOCKET
			SOCKADDR_INET m_SockAddr;
		#endif
	};
}
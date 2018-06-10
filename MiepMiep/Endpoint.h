#pragma once

#include "Platform.h"
#include "Memory.h"
#include "MiepMiep.h"
#include <vector>
using namespace std;


namespace MiepMiep
{
	class ISocket;

	struct AddrMetaDataPair : public ITraceable
	{
		sptr<const IAddress> m_Addr;
		MetaData m_Md;
	};

	class AddressList : public IAddressList, public ITraceable
	{
	public:
		static sptr<AddressList> create();

		MM_TS void addAddress( const IAddress& addr, const MetaData& md ) override;
		MM_TS void removeAddress( const IAddress* addr ) override;
		MM_TS bool readOrWrite( BinSerializer& bs, bool _write ) override;
		MM_TS MetaData metaData(u32 idx) const override;
		MM_TS sptr<const IAddress> address(u32 idx) const override;
		MM_TS u32 count() const override;

	private:
		mutable mutex m_AddressMutex;
		vector<sptr<AddrMetaDataPair>> m_Addresses;
	};


	class Endpoint: public IAddress, public ITraceable
	{
	public:
		Endpoint();

		MM_TS static sptr<Endpoint> createEmpty();
		MM_TS static sptr<Endpoint> createSource( const ISocket& sock, i32* errOut = nullptr );
		MM_TS static sptr<Endpoint> resolve( const string& name, u16 port, i32* errOut=nullptr );
		MM_TS static sptr<Endpoint> fromIpAndPort( const string& ipAndPort, i32* errOut=nullptr );

		// IAddress
		MM_TSC const char* toIpAndPort() const override;
		MM_TSC sptr<IAddress> getCopy() const override;
		MM_TSC sptr<Endpoint> getCopyDerived() const;
		MM_TSC u16 port() const override;

		bool write(class BinSerializer& bs) const override;
		bool read(class BinSerializer& bs) override;
		
		MM_TSC bool operator< ( const Endpoint& other) const;
		MM_TSC bool operator==( const Endpoint& other ) const;
		MM_TSC bool operator!=( const Endpoint& other ) const				{ return !(*this == other); }

		MM_TSC u16 getPortHostOrder() const;
		MM_TSC u16 getPortNetworkOrder() const;
		MM_TSC bool isZero() const;

		// Either ipv4 or ipv6
		MM_TSC byte* getLowLevelAddr();
		MM_TSC const byte* getLowLevelAddr() const;
		MM_TSC u32 getLowLevelAddrSize() const;

	#if _DEBUG
		u16 m_HostPort;
	#endif 

	private:
		#if MM_SDLSOCKET
			IPaddress m_IpAddress;
		#elif  MM_WIN32SOCKET
			SOCKADDR_INET m_SockAddr;
		#endif
	};
}
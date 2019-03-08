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

		void addAddress( const IAddress& addr, const MetaData& md ) override;
		void removeAddress( const IAddress* addr ) override;
		bool readOrWrite( BinSerializer& bs, bool _write ) override;
		MetaData metaData(u32 idx) const override;
		sptr<const IAddress> address(u32 idx) const override;
		u32 count() const override;

	private:
		vector<sptr<AddrMetaDataPair>> m_Addresses;
	};


	class Endpoint: public IAddress, public ITraceable
	{
	public:
		Endpoint();

		static sptr<Endpoint> createEmpty();
		static sptr<Endpoint> createSource( const ISocket& sock, i32* errOut = nullptr );
		static sptr<Endpoint> resolve( const string& name, u16 port, i32* errOut = nullptr );
		static sptr<Endpoint> fromIpAndPort( const string& ipAndPort, i32* errOut = nullptr );

		// IAddress
		const char* toIpAndPort() const override;
		sptr<IAddress> getCopy() const override;
		sptr<Endpoint> getCopyDerived() const;
		u16 port() const override;

		bool write(class BinSerializer& bs) const override;
		bool read(class BinSerializer& bs) override;
		
		bool operator< ( const Endpoint& other ) const;
		bool operator==( const Endpoint& other ) const;
		bool operator!=( const Endpoint& other ) const { return !(*this == other); }

		u16 getPortHostOrder() const;
		u16 getPortNetworkOrder() const;
		bool isZero() const;

		// Either ipv4 or ipv6
		byte* getLowLevelAddr();
		const byte* getLowLevelAddr() const;
		u32 getLowLevelAddrSize() const;

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
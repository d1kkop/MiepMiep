#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class ISocket;
	class Endpoint;
	class Link;
	class SessionBase;
	class Listener;


	struct SocketAddrPair
	{
		SocketAddrPair() = default;
		SocketAddrPair( const ISocket& sock, const IAddress& addr );
		SocketAddrPair( const ISocket& sock, const Endpoint& etp );
		SocketAddrPair( const sptr<const ISocket>& sock, const sptr<const IAddress>& addr );

		sptr<const ISocket>  m_Socket;
		sptr<const IAddress> m_Address;

		MM_TSC operator bool () const;
		MM_TSC bool operator<  (const SocketAddrPair& other) const;
		MM_TSC bool operator== (const SocketAddrPair& other) const;

		MM_TSC const char* info() const;
	};


	class LinkManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		LinkManager(Network& network);
		static EComponentType compType() { return EComponentType::LinkManager; }

		MM_TS sptr<Link> add( SessionBase& session, const SocketAddrPair& sap );
		MM_TS sptr<Link> getOrAdd( SessionBase* session, const SocketAddrPair& sap, bool* wasNew );
		MM_TS sptr<Link> get( const SocketAddrPair& sap );
		MM_TS bool		 has( const SocketAddrPair& sap ) const;
		MM_TS void forEachLink( const std::function<void (Link&)>& cb, u32 clusterSize=0 );

	private:
		vector<sptr<Link>> m_LinksAsArray;
		map<SocketAddrPair, sptr<Link>> m_Links;
	};
}
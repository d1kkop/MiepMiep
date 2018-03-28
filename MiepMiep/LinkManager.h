#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class ISocket;
	class Endpoint;
	class Link;
	class Session;
	class Listener;


	struct SocketAddrPair
	{
		SocketAddrPair() = default;
		SocketAddrPair( const ISocket& sock, const IAddress& addr );
		SocketAddrPair( const ISocket& sock, const Endpoint& etp );
		SocketAddrPair( sptr<ISocket>& sock, sptr<IAddress>& addr );
		SocketAddrPair( sptr<const ISocket>& sock, sptr<const IAddress>& addr );

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

		MM_TS sptr<Link> add( const Session* session, const IAddress& to, bool addHandler );
		MM_TS sptr<Link> add( const Session* session, const IAddress& to, u32 id, bool addHandler );
		MM_TS sptr<Link> add( const Session* session, const SocketAddrPair& sap, u32 id, bool addHandler );
		MM_TS sptr<Link> getOrAdd( const Session* session, const SocketAddrPair& sap, u32 id, bool addHandler, bool returnNullIfIdsDontMatch=true );
		MM_TS sptr<Link> get( const SocketAddrPair& sap );
		MM_TS bool		 has( const SocketAddrPair& sap ) const;
		MM_TS void forEachLink( const std::function<void (Link&)>& cb, u32 clusterSize=0 );

	private:
		mutable mutex m_LinksMapMutex;
		vector<sptr<Link>> m_LinksAsArray;
		map<SocketAddrPair, sptr<Link>> m_Links;
	};
}
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

		sptr<const ISocket>  m_Socket;
		sptr<const IAddress> m_Address;

		operator bool () const;
		bool operator<  (const SocketAddrPair& other) const;
		bool operator== (const SocketAddrPair& other) const;
	};


	class LinkManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		LinkManager(Network& network);
		static EComponentType compType() { return EComponentType::LinkManager; }

		MM_TS sptr<Link> add( const Session* session, const IAddress& to, bool addHandler );
		MM_TS sptr<Link> add( const Session* session, const IAddress& to, u32 id, bool addHandler );
		MM_TS sptr<Link> add( const Session* session, const SocketAddrPair& sap, u32 id, bool addHandler );
		MM_TS sptr<Link> get( const SocketAddrPair& sap );
		MM_TS bool		 has( const SocketAddrPair& sap ) const;
		MM_TS void forEachLink( const std::function<void (Link&)>& cb, u32 clusterSize=0 );
		MM_TS bool forLink( const ISocket& sock, const IAddress* specific, bool exclude, const std::function<void (Link&)>& cb );
		MM_TS sptr<const Link> getFirstLink() const;

	private:
		MM_TS bool tryCreate( sptr<Link>& link, const Session* session, const IAddress& to, u32 id, bool addHandler );
		MM_TS bool tryCreate( sptr<Link>& link, const Session* session, const SocketAddrPair& sap, u32 id, bool addHandler );
		MM_TS void insertNoExistsCheck( const sptr<Link>& link );

	private:
		mutable mutex m_LinksMapMutex;
		vector<sptr<Link>> m_LinksAsArray;
		map<SocketAddrPair, sptr<Link>> m_Links;
	};
}
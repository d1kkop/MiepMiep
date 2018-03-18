#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class ISocket;
	class Endpoint;
	class Link;
	class Listener;


	struct SocketAddrPair
	{
		SocketAddrPair( const ISocket& sock, const IAddress& addr );
		SocketAddrPair( const ISocket& sock, const Endpoint& etp );

		sptr<const ISocket>  m_Socket;
		sptr<const IAddress> m_Address;

		bool operator< (const SocketAddrPair& other) const;
	};


	class LinkManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		LinkManager(Network& network);

		MM_TS sptr<Link> add( const IAddress& to );
		MM_TS sptr<Link> add( u32 id, const Listener& originator, const IAddress& to );
		MM_TS sptr<Link> get( const SocketAddrPair& sap );
		MM_TS void forEachLink( const std::function<void (Link&)>& cb, u32 clusterSize=0 );
		MM_TS bool forLink( const ISocket& sock, const IAddress* specific, bool exclude, const std::function<void (Link&)>& cb );
		MM_TS sptr<const Link> getFirstLink() const;

		// IComponent
		static EComponentType compType() { return EComponentType::LinkManager; }

	private:
		mutable mutex m_LinksMapMutex;
		vector<sptr<Link>> m_LinksAsArray;
		map<SocketAddrPair, sptr<Link>> m_Links;
	};
}
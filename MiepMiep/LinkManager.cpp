#include "LinkManager.h"
#include "Link.h"
#include "Endpoint.h"
#include "Socket.h"
#include "Network.h"
#include "Listener.h"
#include "Util.h"


namespace MiepMiep
{
	// --------- SockAddrPair ------------------------------------------------------------------------

	SocketAddrPair::SocketAddrPair(const ISocket& sock, const Endpoint& etp):
		m_Socket( sock.to_sptr() ),
		m_Address( etp.to_ptr() )
	{
	}

	SocketAddrPair::SocketAddrPair(const ISocket& sock, const IAddress& addr):
		m_Socket( sock.to_sptr() ),
		m_Address( addr.to_ptr() )
	{
	}

	bool SocketAddrPair::operator<(const SocketAddrPair& other) const
	{
		if ( *m_Address < *other.m_Address ) return true;
		if ( !(*m_Address == *other.m_Address) ) return false;
		if ( *m_Socket < *other.m_Socket ) return true;
		if ( !(*m_Socket == *other.m_Socket) ) return false;
		return false;
	}


	// --------- LinkManager ------------------------------------------------------------------------

	LinkManager::LinkManager(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS sptr<Link> LinkManager::add(const IAddress& to)
	{
		return add(to, rand());
	}

	MM_TS sptr<Link> LinkManager::add(const IAddress& to, u32 id)
	{
		sptr<Link> link;
		if ( !tryCreate(link, to, id) )
			return nullptr;
		insertNoExistsCheck(link);
		return link;
	}

	MM_TS sptr<Link> LinkManager::add( const SocketAddrPair& sap, u32 id )
	{
		sptr<Link> link;
		if ( !tryCreate( link, sap, id ) )
			return nullptr;
		insertNoExistsCheck( link );
		return link;
	}

	MM_TS sptr<Link> LinkManager::get( const SocketAddrPair& sap )
	{
		scoped_lock lk(m_LinksMapMutex);
		auto lit = m_Links.find( sap );
		if ( lit != m_Links.end() )
		{
			return lit->second;
		}
		return nullptr;
	}

	MM_TS bool LinkManager::has( const SocketAddrPair& sap ) const
	{
		scoped_lock lk( m_LinksMapMutex );
		return m_Links.count( sap ) != 0;
	}

	MM_TS void LinkManager::forEachLink( const std::function<void( Link& )>& cb, u32 clusterSize )
	{
		if ( 0 == clusterSize )
		{
			scoped_lock lk(m_LinksMapMutex);
			for ( auto& link : m_LinksAsArray )
			{
				cb( *link );
			}
		}
		else
		{
			Util::cluster( m_LinksAsArray.size(), clusterSize, [&]( auto s, auto e )
			{
				while ( s < e )
				{
					cb( *m_LinksAsArray[s] );
					++s;
				}
			});
		}
	}

	MM_TS bool LinkManager::forLink(const ISocket& sock, const IAddress* specific, bool exclude, const std::function<void(Link&)>& cb)
	{
		bool wasSentAtLeastOnce = false;
		scoped_lock lk(m_LinksMapMutex);
		if ( specific ) 
		{
			if ( !exclude )
			{
				SocketAddrPair sap( sock, *specific );
				auto linkIt = m_Links.find( sap );
				if ( linkIt != m_Links.end() )
				{
					cb ( *linkIt->second );
					wasSentAtLeastOnce = true;
				}
			}
			else
			{
				for ( auto& link : m_LinksAsArray )
				{
					if ( !(link->socket() == sock || m_Network.isListenerSocket( sock )) || link->destination() == *specific ) 
						continue; // skip this one
					cb ( *link );
					wasSentAtLeastOnce = true;
				}
			}
		}
		else
		{
			for ( auto& link : m_LinksAsArray )
			{
				if ( !(link->socket() == sock || m_Network.isListenerSocket( sock )) )
					continue; 
				cb ( *link );
				wasSentAtLeastOnce = true;
			}
		}
		return wasSentAtLeastOnce;
	}

	MM_TS sptr<const Link> LinkManager::getFirstLink() const
	{
		scoped_lock lk(m_LinksMapMutex);
		if ( !m_LinksAsArray.empty() )
			return m_LinksAsArray[0];
		return nullptr;
	}

	MM_TS bool LinkManager::tryCreate( sptr<Link>& link, const IAddress& to, u32 id )
	{
		link = Link::create( m_Network, to, id );
		if ( !link ) return false;
		auto sap = link->getSocketAddrPair();
		if ( has( sap ) )
		{
			LOGW( "Tried to create a link that does already exists, creation discarded." );
			return false;
		}
		link = Link::create( m_Network, sap, id );
		return link != nullptr;
	}

	MM_TS bool LinkManager::tryCreate( sptr<Link>& link, const SocketAddrPair& sap, u32 id )
	{
		if ( has( sap ) )
		{
			LOGW( "Tried to create a link that does already exists, creation discarded." );
			return false;
		}
		link = Link::create( m_Network, sap, id );
		return link != nullptr;
	}

	MM_TS void LinkManager::insertNoExistsCheck( const sptr<Link>& link )
	{
		scoped_lock lk( m_LinksMapMutex );
		auto sap = link->getSocketAddrPair();
		assert( m_Links.count(sap) == 0 );
		m_Links[sap] = link;
		m_LinksAsArray.emplace_back( link );
	}

}
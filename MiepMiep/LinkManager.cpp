#include "LinkManager.h"
#include "Link.h"
#include "Endpoint.h"
#include  "Socket.h"
#include "Util.h"


namespace MiepMiep
{
	// --------- SockAddrPair ------------------------------------------------------------------------

	SocketAddrPair::SocketAddrPair(const ISocket* sock, const IAddress& addr):
		m_Socket( sock ? sock->to_ptr() : nullptr ),
		m_Address( addr.to_ptr() )
	{
	}

	SocketAddrPair::SocketAddrPair(const ISocket* sock, const Endpoint& etp):
		m_Socket( sock ? sock->to_ptr() : nullptr ),
		m_Address( etp.to_ptr() )
	{
	}

	SocketAddrPair::SocketAddrPair(const ISocket& sock, const Endpoint& etp):
		SocketAddrPair( &sock, etp )
	{
	}

	SocketAddrPair::SocketAddrPair(const ISocket& sock, const IAddress& addr):
		SocketAddrPair( &sock, addr )
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

	MM_TS sptr<Link> LinkManager::getOrAdd(const SocketAddrPair& sap, u32* id, const Listener* originator, bool* wasAdded)
	{
		scoped_lock lk(m_LinksMapMutex);

		if ( m_Links2.count( sap ) != 0 ) 
		{
			if (wasAdded) *wasAdded = false;
			return m_Links2[sap];
		}

		// create fail
		sptr<Link> link = Link::create(m_Network, *sap.m_Address, id, originator);
		if (!link)
		{
			if (wasAdded) *wasAdded = false;
			return nullptr;
		}

		// insert
		m_Links2[link->extractSocketAddrPair()] = link;
		m_LinksAsArray.emplace_back( link );
		if ( wasAdded ) *wasAdded = true;

		return link;
	}

	MM_TS sptr<Link> LinkManager::getLink(const SocketAddrPair& sap)
	{
		scoped_lock lk(m_LinksMapMutex);
		auto lit = m_Links2.find( sap );
		if ( lit != m_Links2.end() )
		{
			return lit->second;
		}
		return nullptr;
	}

	MM_TS void LinkManager::forEachLink(const std::function<void(Link&)>& cb, u32 clusterSize)
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
				auto linkIt = m_Links2.find( sap );
				if ( linkIt != m_Links2.end() ) 
				{
					cb ( *linkIt->second );
					wasSentAtLeastOnce = true;
				}
			}
			else
			{
				for ( auto& link : m_LinksAsArray )
				{
					if ( !(link->socket() == sock || isListenerSocket( sock )) || link->destination() == *specific ) 
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
				if ( !(link->socket() == sock || isListernerSocket( sock )) )
					continue; 
				cb ( *link );
				wasSentAtLeastOnce = true;
			}
		}
		return wasSentAtLeastOnce;
	}
}
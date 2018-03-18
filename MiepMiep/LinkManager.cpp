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
		sptr<Link> link = Link::create(m_Network, to);
		if (!link)
		{
			return nullptr;
		}

		// insert
		scoped_lock lk(m_LinksMapMutex);
		m_Links[link->extractSocketAddrPair()] = link;
		m_LinksAsArray.emplace_back(link);

		return link;
	}

	MM_TS sptr<Link> LinkManager::add(u32 id, const Listener& originator, const IAddress& to)
	{
		SocketAddrPair sap( originator.socket(), to );
		scoped_lock lk(m_LinksMapMutex);
		if ( m_Links.count( sap ) != 0 )
			return nullptr; // already exists

		sptr<Link> link = Link::create(m_Network, id, originator, to);
		if (!link)
		{
			return nullptr;
		}

		// insert
		m_Links[link->extractSocketAddrPair()] = link;
		m_LinksAsArray.emplace_back(link);

		return link;
	}

	MM_TS sptr<Link> LinkManager::get(const SocketAddrPair& sap)
	{
		scoped_lock lk(m_LinksMapMutex);
		auto lit = m_Links.find( sap );
		if ( lit != m_Links.end() )
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

}
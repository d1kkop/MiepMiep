#include "LinkManager.h"
#include "Link.h"
#include "Endpoint.h"
#include "Socket.h"
#include "Network.h"
#include "Listener.h"
#include "ListenerManager.h"
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

	//SocketAddrPair::SocketAddrPair( sptr<ISocket>& sock, sptr<IAddress>& addr ):
	//	m_Socket( sock ),
	//	m_Address( addr )
	//{
	//}

	SocketAddrPair::SocketAddrPair( const sptr<const ISocket>& sock, const sptr<const IAddress>& addr ):
		m_Socket(sock),
		m_Address(addr)
	{
	}

	MM_TSC SocketAddrPair::operator bool() const
	{
		return m_Socket && m_Address;
	}

	MM_TSC bool SocketAddrPair::operator<( const SocketAddrPair& other ) const
	{
		if ( *m_Address == *other.m_Address )
			return *m_Socket < *other.m_Socket;
		return *m_Address < *other.m_Address;
	}


	MM_TSC const char* SocketAddrPair::info() const
	{
		static thread_local char buff[80];
		Platform::formatPrint( buff, 80, "s: %d a: %s", m_Socket?m_Socket->id():-1, m_Address?m_Address->toIpAndPort():"" );
		return buff;
	}

	MM_TSC bool SocketAddrPair::operator==( const SocketAddrPair& other ) const
	{
		return  (*m_Address == *other.m_Address) &&
				(*m_Socket == *other.m_Socket);
	}


	// --------- LinkManager ------------------------------------------------------------------------

	LinkManager::LinkManager(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS sptr<Link> LinkManager::add( SessionBase& session, const SocketAddrPair& sap, u32 id, bool addHandler, bool addToSession )
	{
		sptr<Link> link = Link::create( m_Network, session, sap, id, addHandler, addToSession );
		if ( !link ) return nullptr;
		scoped_lock lk( m_LinksMapMutex );
		if ( m_Links.count( sap ) != 0 )
		{
			LOGW( "Tried to create a link that does already exist, creation discarded." );
			return nullptr;
		}
		assert( link );
		m_LinksAsArray.emplace_back(link);
		m_Links[sap] = link;
		return link;
	}

	MM_TS sptr<Link> LinkManager::getOrAdd( SessionBase* session, const SocketAddrPair& sap, u32 id, bool addHandler, bool addToSession, bool returnNullIfIdsDontMatch, bool* wasNew )
	{
		scoped_lock lk( m_LinksMapMutex );
		//LOG( "List links..." );
		//for ( auto& kvp : m_Links )
		//{
		//	LOG( "In map: key %s, value %s.", kvp.first.info(), kvp.second->info() );
		//}

		//for ( auto& l : m_LinksAsArray )
		//{
		//	if ( l->getSocketAddrPair() == sap )
		//	{
		//		LOG( "Wanted %s, Found %s.", sap.info(), l->info() );
		//		return l;
		//	}
		//}

		auto lIt  = m_Links.find( sap );
		if ( lIt != m_Links.end() )
		{
			if (wasNew) *wasNew = false;

		//	LOG("Wanted %s, Found %s.", sap.info(), lIt->second->info());
			const sptr<Link>& l = lIt->second;
			if ( !returnNullIfIdsDontMatch || id == l->id() )
			{
				return l;
			}
			// Did find, but id's did not match.
		//	LOG( "FAILED to find %s id's did not match.", sap.info() );
			assert(false);
			return nullptr;
		}

		if ( wasNew ) *wasNew = true;

	//	LOG( "FAILED to find %s", sap.info() );
		sptr<Link> link = Link::create( m_Network, session, sap, id, addHandler, addToSession );
		if ( !link )
		{
			assert(false);
			return nullptr;
		}

		m_LinksAsArray.emplace_back( link );
		m_Links[sap] = link;
		return link;
	}

	MM_TS sptr<Link> LinkManager::get( const SocketAddrPair& sap )
	{
		scoped_lock lk(m_LinksMapMutex);

		//LOG("List links...");
		//for ( auto& kvp : m_Links )
		//{
		//	LOG( "In map: key %s, value %s.", kvp.first.info(), kvp.second->info() );
		//}

		auto lit = m_Links.find( sap );
		if ( lit != m_Links.end() )
		{
	//		LOG("Wanted %s, Found %s.", sap.info(), lit->second->info());
			return lit->second;
		}

	//	LOG( "FAILED to find %s.", sap.info() );
		return nullptr;
	}

	MM_TS bool LinkManager::has( const SocketAddrPair& sap ) const
	{
		scoped_lock lk( m_LinksMapMutex );
		return m_Links.count( sap ) != 0;
	}

	// TODO make actually parallel
	MM_TS void LinkManager::forEachLink( const std::function<void( Link& )>& cb, u32 clusterSize )
	{
		// Only obtain lock for extracting link from list. Do not hold it.
		for ( u32 i=0; i < UINT_MAX; i++ )
		{
			sptr<Link> link;
			{
				scoped_lock lk( m_LinksMapMutex );
				if ( i >= m_LinksAsArray.size() )
					break;
				link = m_LinksAsArray[i];
			}
			if ( link )
			{
				cb ( *link );
			}
		}
	}
}
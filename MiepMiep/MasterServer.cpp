#include "MasterServer.h"


namespace MiepMiep
{
	// ------ MasterSession ----------------------------------------------------------------------------

	MasterSession::MasterSession( bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customMd ):
		m_IsP2p(isP2p),
		m_Name(name),
		m_Type(type),
		m_Rating(initialRating),
		m_CustomMd(customMd),
		// -- The below will be updated from host --
		m_IsPrivate(true), // Start private, have host update this.
		m_MaxClients(0)
	{

	}

	MM_TS bool MasterSession::operator==( const SearchFilter& sf ) const
	{
		scoped_lock lk( m_DataMutex );
		return
			m_Name == sf.m_Name &&
			m_Type == sf.m_Type &&
			(!m_IsPrivate || sf.m_FindPrivate) &&
			((m_IsP2p && sf.m_FindP2p) || (!m_IsP2p && sf.m_FindClientServer)) &&
			((u32)m_Links.size() >= sf.m_MinPlayers &&  sf.m_MaxPlayers <= m_MaxClients) &&
			(m_Rating >= sf.m_MinRating && m_Rating <= sf.m_MaxRating);
	}


	// ------ ServerList ----------------------------------------------------------------------------

	void ServerList::add(const SocketAddrPair& sap, bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customFilterMd )
	{
		scoped_lock lk(m_ServersMutex);
		assert( m_Servers.count( sap ) == 0 );
		m_Servers.try_emplace( sap, isP2p, name, type, initialRating, customFilterMd );
	}

	bool ServerList::exists(const SocketAddrPair& sap) const
	{
		scoped_lock lk(m_ServersMutex);
		return m_Servers.count( sap ) == 1;
	}

	u64 ServerList::num() const
	{
		scoped_lock lk(m_ServersMutex);
		return m_Servers.size();
	}

	MM_TS SocketAddrPair ServerList::findFromFilter(const SearchFilter& sf)
	{
		scoped_lock lk(m_ServersMutex);
		for ( auto& kvp : m_Servers )
		{
			const MasterSession& se = kvp.second;
			if ( se == sf )
			{
				return kvp.first;
			}					 
		}
		return SocketAddrPair();
	}

	// ------ MasterServer ----------------------------------------------------------------------------

	MasterServer::MasterServer(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS bool MasterServer::registerServer(const ISocket& reception, const IAddress& addr, bool isP2p, const string& name, 
											const string& type, float initialRating, const MetaData& customFilterMd )
	{
		SocketAddrPair sap( reception, addr );
		scoped_lock lk(m_ServerListMutex);

		for ( auto& sl : m_ServerLists )
		{
			if ( sl->exists( sap ) ) 
				return false;
		}

		for ( auto& sl : m_ServerLists )
		{
			if ( sl->num() < MM_NEW_SERVER_LIST_THRESHOLD )
			{
				sl->add( sap, isP2p, name, type, initialRating, customFilterMd );
				return true;
			}
		}

		// not added as all have maximum size reached, add new server list
		auto sl = make_shared<ServerList>();
		sl->add( sap, isP2p, name, type, initialRating, customFilterMd );
		m_ServerLists.emplace_back( sl );

		return true;
	}

	MM_TS SocketAddrPair MasterServer::findServerFromFilter(const SearchFilter& sf)
	{
		// Because servers are split among server lists, we do not need to lock all servers, only the lists of servers.
		u32 i=0;
		for (u32 i=0; i != UINT_MAX; i++)
		{
			sptr<ServerList> sl;

			// Try obtain a server list to search trough.
			{
				scoped_lock  lk(m_ServerListMutex);
				if ( i < m_ServerLists.size() )
				{
					sl = m_ServerLists[i];
				}
				else break; // no server list remaining
			}

			if (sl)
			{
				auto sap = sl->findFromFilter( sf );
				if ( sap ) return sap;
			}
			else
			{
				LOGW("Obtained a nullptr server list.");
				break;
			}
		}

		return SocketAddrPair();
	}

}
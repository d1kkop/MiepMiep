#include "MasterServer.h"


namespace MiepMiep
{
	// ------ ServerEntry ----------------------------------------------------------------------------

	ServerEntry::ServerEntry(const string& name, const string& pw, const string& type, float initialRating, const MetaData& customFilterMd):
		m_Name(name),
		m_Pw(pw),
		m_Type(type),
		m_Rating(initialRating),
		m_NumPlayers(0),
		m_CustomFilterMd(customFilterMd)
	{
	}


	// ------ ServerList ----------------------------------------------------------------------------

	void ServerList::add(const SocketAddrPair& sap, const string& name, const string& pw, const string& type, float initialRating, const MetaData& customFilterMd )
	{
		scoped_lock lk(m_ServersMutex);
		assert( m_Servers.count( sap ) == 0 );
		m_Servers[sap] = ServerEntry( name, pw, type, initialRating, customFilterMd );
	}

	bool ServerList::exists(const SocketAddrPair& sap) const
	{
		scoped_lock lk(m_ServersMutex);
		return m_Servers.count( sap ) == 1;
	}

	u64 ServerList::count() const
	{
		scoped_lock lk(m_ServersMutex);
		return m_Servers.size();
	}

	MM_TS SocketAddrPair ServerList::findFromFilter(const string& name, const string& pw, const string& type, float minRating, float maxRating, u32 minPlayers, u32 maxPlayers)
	{
		scoped_lock lk(m_ServersMutex);
		for ( auto& kvp : m_Servers )
		{
			const ServerEntry& se = kvp.second;
			if ( se.m_Name == name && 
				 se.m_Pw == pw &&
				 se.m_Type == type &&
				 (se.m_NumPlayers >= minPlayers && se.m_NumPlayers <= maxPlayers) && 
				 (se.m_Rating >= minRating && se.m_Rating <= maxRating) )
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

	MM_TS bool MasterServer::registerServer(const ISocket& reception, const IAddress& addr, const string& name, 
											const string& pw, const string& type, float initialRating, const MetaData& customFilterMd )
	{
		SocketAddrPair sap( reception, addr );
		scoped_lock lk(m_ServersMutex);

		for ( auto& sl : m_ServerLists )
		{
			if ( sl->exists( sap ) ) 
				return false;
		}

		for ( auto& sl : m_ServerLists )
		{
			if ( sl->count() < MM_NEW_SERVER_LIST_THRESHOLD )
			{
				sl->add( sap, name, pw, type, initialRating, customFilterMd );
				return true;
			}
		}

		// not added as all have maximum size reached, add new server list
		auto sl = make_shared<ServerList>();
		sl->add( sap, name, pw, type, initialRating, customFilterMd );
		m_ServerLists.emplace_back( sl );

		return true;
	}

	MM_TS SocketAddrPair MasterServer::findServerFromFilter(const string& name, const string& pw, const string& type,
															float minRating, float maxRating, u32 minPlayers, u32 maxPlayers)
	{
		// Because servers are split among server lists, we do not need to lock all servers, only the lists of servers.
		u32 i=0;
		while (i != UINT_MAX)
		{
			sptr<ServerList> sl;

			// Try obtain a server list to search trough.
			{
				scoped_lock  lk(m_ServersMutex);
				if ( i < m_ServerLists.size() )
				{
					sl = m_ServerLists[i];
				}
				else break; // no server list remaining
			}

			if (sl)
			{
				return sl->findFromFilter( name, pw, type, minRating, maxRating, minPlayers, maxPlayers );
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
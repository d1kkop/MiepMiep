#include "MasterServer.h"
#include "Link.h"


namespace MiepMiep
{

	// ------ MasterSession ----------------------------------------------------------------------------

	MasterSession::MasterSession( const MasterSessionData& data ):
		m_Data(data)
	{
	}

	MM_TS bool MasterSession::operator==( const SearchFilter& sf ) const
	{
		scoped_lock lk( m_DataMutex );
		auto& d = m_Data;
		return
			d.m_Name == sf.m_Name &&
			d.m_Type == sf.m_Type &&
			(!d.m_IsPrivate || sf.m_FindPrivate) &&
			((d.m_IsP2p && sf.m_FindP2p) || (!d.m_IsP2p && sf.m_FindClientServer)) &&
			((u32)m_Links.size() >= sf.m_MinPlayers &&  sf.m_MaxPlayers <= d.m_MaxClients) &&
			(d.m_Rating >= sf.m_MinRating && d.m_Rating <= sf.m_MaxRating);
	}


	// ------ ServerList ----------------------------------------------------------------------------

	void ServerList::addNewMasterSession(const SocketAddrPair& sap, const MasterSessionData& data)
	{
		scoped_lock lk(m_ServersMutex);
		assert( m_MasterSessions.count( sap ) == 0 );
		m_MasterSessions.try_emplace( sap, data );
	}

	bool ServerList::exists(const SocketAddrPair& sap) const
	{
		scoped_lock lk(m_ServersMutex);
		return m_MasterSessions.count( sap ) == 1;
	}

	u64 ServerList::num() const
	{
		scoped_lock lk(m_ServersMutex);
		return m_MasterSessions.size();
	}

	MM_TS SocketAddrPair ServerList::findFromFilter(const SearchFilter& sf)
	{
		scoped_lock lk(m_ServersMutex);
		for ( auto& kvp : m_MasterSessions )
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

	MM_TS bool MasterServer::registerServer(const sptr<Link>& link, const MasterSessionData& data)
	{
		auto sap = link->getSocketAddrPair();
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
				sl->addNewMasterSession( sap, data );
				return true;
			}
		}

		// not added as all have maximum size reached, add new server list
		auto sl = make_shared<ServerList>();
		sl->addNewMasterSession( sap, data );
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
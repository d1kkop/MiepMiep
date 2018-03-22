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


	// ------ ServerList -----------------------------------------------------------------------------------

	void ServerList::addServer(const sptr<MasterSessionData>& s_data)
	{
		scoped_lock lk(m_ServersMutex);
		assert( m_MasterSessions.count(s_data->m_Id) == 0 );
		s_data->m_ServerList = ptr<ServerList>();
		m_MasterSessions.try_emplace( s_data->m_Id, s_data );
	}

	MM_TS void ServerList::removeServer( const sptr<MasterSessionData>& s_data )
	{
		scoped_lock lk( m_ServersMutex );
		m_MasterSessions.erase( s_data->m_Id );
	}

	u64 ServerList::num() const
	{
		scoped_lock lk(m_ServersMutex);
		return m_MasterSessions.size();
	}

	MM_TS sptr<MasterSession> ServerList::findFromFilter(const SearchFilter& sf)
	{
		scoped_lock lk(m_ServersMutex);
		for ( auto& kvp : m_MasterSessions )
		{
			if ( *kvp.second == sf )
			{
				return kvp.second;
			}					 
		}
		return nullptr;
	}

	// ------ MasterServer ----------------------------------------------------------------------------

	MasterServer::MasterServer(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS bool MasterServer::registerServer(const sptr<Link>& link, const MasterSessionData& data)
	{
		if ( link->has<MasterSessionData>() )
		{
			return false;
		}

		// Create new shared master session data. Shared by all links in the session.
		sptr<MasterSessionData> s_data = link->getOrAdd<MasterSessionData>( data );
		s_data->m_Id = m_MasterSessionIdNumerator++;

		scoped_lock lk(m_ServerListMutex);
		for ( auto& sl : m_ServerLists )
		{
			if ( sl->num() < MM_NEW_SERVER_LIST_THRESHOLD )
			{
				sl->addServer( s_data );
				return true;
			}
		}

		// Not added as all have maximum size reached, add new server list
		auto sl = reserve_sp<ServerList>(MM_FL);
		sl->addServer( s_data );
		m_ServerLists.emplace_back( sl );

		return true;
	}

	MM_TS void MasterServer::removeServer( const sptr<MasterSession>& s_data )
	{
		s_data->m_ServerList->removeServer( s_data );
	}

	MM_TS SocketAddrPair MasterServer::findServerFromFilter( const SearchFilter& sf )
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
				auto m_session = sl->findFromFilter( sf );
				if ( m_session ) return m_session;
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
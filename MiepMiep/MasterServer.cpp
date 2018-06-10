#include "MasterServer.h"
#include "MasterSession.h"


namespace MiepMiep
{
	// ------ Custom Serialization --------------------------------------------------------------------------------

	template <>
	bool readOrWrite( BinSerializer& bs, SearchFilter& sf, bool _write )
	{
		__CHECKEDB( bs.readOrWrite( sf.m_Type, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_Name, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MinRating, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MinPlayers, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MaxRating, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MaxPlayers, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_FindPrivate, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_FindP2p, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_FindClientServer, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_CustomMatchmakingMd, _write ) );
		return true;
	}


	// ------ SessionList -----------------------------------------------------------------------------------

	MasterSessionList::~MasterSessionList()
	{
		LOG( "Destroying master session list." );
	}

	MM_TS sptr<MasterSession> MasterSessionList::addSession( const sptr<Link>& host, const MasterSessionData& data )
	{
		sptr<MasterSession> session = reserve_sp<MasterSession>( MM_FL, host, data, *this );
		scoped_lock lk(m_SessionsMutex);
		m_MasterSessions.emplace_back( session );
		session->m_SessionListIt = prev( m_MasterSessions.end() );
		return session;
	}

	MM_TS void MasterSessionList::removeSession( MsListCIt whereIt )
	{
		scoped_lock lk( m_SessionsMutex );
		m_MasterSessions.erase( whereIt );
	}

	u64 MasterSessionList::num() const
	{
		scoped_lock lk(m_SessionsMutex);
		return m_MasterSessions.size();
	}

	MM_TS sptr<MasterSession> MasterSessionList::findFromFilter(const SearchFilter& sf)
	{
		scoped_lock lk(m_SessionsMutex);
		for ( auto& ms : m_MasterSessions )
		{
			if ( *ms == sf )
			{
				return ms;
			}					 
		}
		return nullptr;
	}


	// ------ MasterServer ----------------------------------------------------------------------------

	MasterServer::MasterServer(Network& network):
		ParentNetwork(network)
	{
	}

	MasterServer::~MasterServer()
	{
		LOG("Destroying master server.");
	}

	MM_TS sptr<MasterSession> MasterServer::registerSession( const sptr<Link>& link, const MasterSessionData& data )
	{
		sptr<MasterSessionList> sessionList;

		// Obtain list
		{
			scoped_lock lk( m_ServerListMutex );
			for ( auto& sl : m_SessionLists )
			{
				if ( sl->num() < MM_NEW_SERVER_LIST_THRESHOLD )
				{
					sessionList = sl;
					break;
				}
			}
			if ( !sessionList )
			{
				sessionList = reserve_sp<MasterSessionList>( MM_FL );
				m_SessionLists.emplace_back( sessionList );
			}
		}

		// No longer need lock for session list. Add session.
		assert( sessionList );
		return sessionList->addSession( link, data );
	}

	MM_TS void MasterServer::removeSession( MasterSession& session )
	{
		session.removeSelf();
	}

	MM_TS sptr<MasterSession> MasterServer::findServerFromFilter( const SearchFilter& sf )
	{
		// Because servers are split among server lists, we do not need to lock all servers, only the lists of servers.
		u32 i=0;
		for (u32 i=0; i != UINT_MAX; i++)
		{
			sptr<MasterSessionList> sl;

			// Try obtain a server list to search trough.
			{
				scoped_lock  lk(m_ServerListMutex);
				if ( i < m_SessionLists.size() )
				{
					sl = m_SessionLists[i];
				}
				else break; // no server list remaining
			}

			if (sl)
			{
				auto session = sl->findFromFilter( sf );
				if ( session ) return session;
			}
			else
			{
				LOGW("Obtained a nullptr server list.");
				break;
			}
		}

		return nullptr;
	}

}
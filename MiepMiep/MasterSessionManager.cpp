#include "MasterSessionManager.h"
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

	sptr<MasterSession> MasterSessionList::addSession( const sptr<Link>& host, const MasterSessionData& data )
	{
		sptr<MasterSession> session = reserve_sp<MasterSession>( MM_FL, host, data, *this );
		m_MasterSessions.emplace_back( session );
		session->m_SessionListIt = prev( m_MasterSessions.end() );
		return session;
	}

	void MasterSessionList::removeSession( MsListCIt whereIt )
	{
		m_MasterSessions.erase( whereIt );
	}

	u64 MasterSessionList::num() const
	{
		return m_MasterSessions.size();
	}

	sptr<MasterSession> MasterSessionList::findFromFilter(const SearchFilter& sf)
	{
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

	MasterSessionManager::MasterSessionManager(Network& network):
		ParentNetwork(network)
	{
	}

	MasterSessionManager::~MasterSessionManager()
	{
		LOG("Destroying master server.");
	}

	sptr<MasterSession> MasterSessionManager::registerSession( const sptr<Link>& link, const MasterSessionData& data )
	{
		sptr<MasterSessionList> sessionList;

		// Obtain list
		{
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

	void MasterSessionManager::removeSession( MasterSession& session )
	{
		session.removeSelf();
	}

	sptr<MasterSession> MasterSessionManager::findServerFromFilter( const SearchFilter& sf )
	{
		// Because servers are split among server lists, we do not need to lock all servers, only the lists of servers.
		u32 i=0;
		for (u32 i=0; i != UINT_MAX; i++)
		{
			sptr<MasterSessionList> sl;

			// Try obtain a server list to search trough.
			{
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
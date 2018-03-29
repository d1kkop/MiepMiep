#include "MasterServer.h"
#include "Link.h"
#include "Session.h"
#include "Network.h"
#include "LinkStats.h"
#include "LinkState.h"
#include "NetworkEvents.h"
#include <algorithm>


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

	template <>
	bool readOrWrite( BinSerializer& bs, MasterSessionData& md, bool _write )
	{
		__CHECKEDB( bs.readOrWrite( md.m_IsP2p, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_UsedMatchmaker, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_CanJoinAfterStart, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_IsPrivate, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_Rating, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_MaxClients, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_Name, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_Type, _write ) );
		__CHECKEDB( bs.readOrWrite( md.m_Password, _write ) );
		return true;
	}


	// ------ Event --------------------------------------------------------------------------------

	struct EventNewHost : IEvent
	{
		EventNewHost( const sptr<Link>& link, const sptr<const IAddress>& newHost ):
			IEvent( link, false ),
			m_NewHost( newHost )
		{
		}

		void process() override
		{
			m_Link->ses().forListeners( [&]( ISessionListener* l )
			{
				// If empty, we become host.
				l->onNewHost( m_Link->session(), m_NewHost.get() );
			} );
		}

		sptr<const IAddress> m_NewHost;
	};


	// ------ RPC --------------------------------------------------------------------------------

	// Executed on client.
	MM_RPC( masterSessionConnectTo, u32, sptr<IAddress> )
	{
		RPC_BEGIN();
		u32 linkId = get<0>(tp);
		const sptr<IAddress>& toAddr = get<1>(tp);
		sptr<Link> newLink = nw.getOrAdd<LinkManager>()->add( &s, *toAddr, linkId, false /*addHandler*/ );
		if ( newLink )
		{
			newLink->getOrAdd<LinkState>()->connect( s.metaData() );
		}
	}

	// Executed on client.
	MM_RPC( masterSessionNewHost, sptr<IAddress> )
	{
		RPC_BEGIN();
		const sptr<IAddress>& host = get<0>( tp );
		l.pushEvent<EventNewHost>( host );
	}


	// ------ MasterSession ----------------------------------------------------------------------------

	MasterSession::MasterSession( const sptr<Link>& host, const MasterSessionData& data, const MasterSessionList& sessionList, const MetaData& customMatchmakingMd ):
		m_Started(false),
		m_Host(host),
		m_Data(data),
		m_SessionList( sessionList.ptr<MasterSessionList>() ),
		m_NewLinksCanJoin(true)
	{
		host->updateCustomMatchmakingMd( customMatchmakingMd );
		m_Links.emplace_back( host );
	}

	MM_TS bool MasterSession::start()
	{
		scoped_lock lk( m_DataMutex);
		if ( !m_Started )
		{
			m_Started = true;
			return true;
		}
		return false;
	}

	MM_TS bool MasterSession::tryJoin( Link& newLink, const MetaData& customMatchmakingMd )
	{
		newLink.updateCustomMatchmakingMd( customMatchmakingMd );
		scoped_lock lk( m_DataMutex );
		if ( !canJoinNoLock() )
			return false;
		newLink.setMasterSession( ptr<MasterSession>() );
		if ( m_Data.m_IsP2p )
		{
			// Have all existing links connect to new link and new link connect to all existing links.
			auto addrCpy = newLink.destination().getCopy();
			for ( auto& l : m_Links )
			{
				u32 linkId = rand();
				l->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, addrCpy );
				newLink.callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, l->destination().getCopy() );
			}
		}
		else
		{
			assert(m_Host); // Can join only returns true, if session has host.
			// For client-server architecture, only have new client join to host.
			u32 linkId = rand();
			m_Host->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, newLink.destination().getCopy() );
			newLink.callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, m_Host->destination().getCopy() );
		}
		m_Links.emplace_back( newLink.to_ptr() );
		assert( m_Host ); // Can join only returns true, if session has host.
		newLink.callRpc<masterSessionNewHost, sptr<IAddress>>( m_Host->destination().getCopy() );
		return true;
	}

	MM_TS bool MasterSession::onClientLeaves( Link& link )
	{
		bool removeSessionOnReturn = false;
		scoped_lock lk( m_DataMutex );

		// TODO use map instead? 
	#if _DEBUG
		auto lIt = std::find_if( begin( m_Links ), end( m_Links ), [&] ( auto& l ) { return *l==link; });
		if ( lIt == m_Links.end() )
		{
			assert(false);
			LOGW( "Link %s not found in master session.", link.info() );
		}
	#endif

		// If is P2p and session already started, new links can no longer join if someone left because
		// we do not have all addresses anymore that might be in the (game)session.
		if ( m_Data.m_IsP2p && m_Started )
		{
			m_NewLinksCanJoin = false;
		}

		// If leaving link is host, determine new host.
		if ( link == *m_Host )
		{
			u32 highestScore = 0;
			m_Host.reset();
			for ( auto& l : m_Links )
			{
				if ( *l == link ) continue; // skip leaving host
				u32 score = l->getOrAdd<LinkStats>()->hostScore();
				if ( score > highestScore )
				{
					highestScore = score;
					m_Host = l;
				}
			}
			if ( !m_Host )
			{
				// Session, has no links left, have it remove itself.
				assert(m_Links.empty());
				removeSessionOnReturn = true;
			}
			else
			{
				link.network().callRpc<masterSessionNewHost, sptr<IAddress>>(
					m_Host->destination().getCopy(), &link.session(), &link, 
					No_Local, No_Buffer, No_Relay, MM_RPC_CHANNEL, No_Trace );
			}
		}

		std::remove_if( begin( m_Links ), end( m_Links ), [&] ( auto l ) { return *l==link; } );
		return removeSessionOnReturn;
	}

	MM_TS void MasterSession::removeSelf()
	{
		scoped_lock lk( m_DataMutex );
		if ( m_SessionList )
		{
			m_SessionList->removeSession( m_SessionListIt );
			m_SessionList.reset();
		}
	}

	MM_TS bool MasterSession::operator==( const SearchFilter& sf ) const
	{
		scoped_lock lk( m_DataMutex );
		auto& d = m_Data;
		return
			canJoinNoLock() && 
			d.m_Name == sf.m_Name &&
			d.m_Type == sf.m_Type &&
			(!d.m_IsPrivate || sf.m_FindPrivate) &&
			((d.m_IsP2p && sf.m_FindP2p) || (!d.m_IsP2p && sf.m_FindClientServer)) &&
			((u32)m_Links.size() >= sf.m_MinPlayers && d.m_MaxClients <= sf.m_MaxPlayers) &&
			(d.m_Rating >= sf.m_MinRating && d.m_Rating <= sf.m_MaxRating);
	}

	bool MasterSession::canJoinNoLock() const
	{
		// Becomes false if session was started in p2p and someone left or
		// if was started and no late join is allowed.
		return m_Host && m_NewLinksCanJoin && (!m_Started || m_Data.m_CanJoinAfterStart);
	}


	// ------ SessionList -----------------------------------------------------------------------------------

	MM_TS void MasterSessionList::addSession(const sptr<Link>& host, const MasterSessionData& data, const MetaData& customMatchmakingMd )
	{
		auto sharedSession = reserve_sp<MasterSession>( MM_FL, host, data, *this, customMatchmakingMd );
		host->setMasterSession( sharedSession );
		scoped_lock lk(m_SessionsMutex);
		m_MasterSessions.emplace_back( sharedSession );
		sharedSession->m_SessionListIt = prev( m_MasterSessions.end() );
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

	MM_TS MasterSession* MasterSessionList::findFromFilter(const SearchFilter& sf)
	{
		scoped_lock lk(m_SessionsMutex);
		for ( auto& ms : m_MasterSessions )
		{
			if ( *ms == sf )
			{
				return ms.get();
			}					 
		}
		return nullptr;
	}

	// ------ MasterServer ----------------------------------------------------------------------------

	MasterServer::MasterServer(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS bool MasterServer::registerSession(const sptr<Link>& link, const MasterSessionData& data, const MetaData& customMatchmakingMd )
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
		sessionList->addSession( link, data, customMatchmakingMd );

		return true;
	}

	MM_TS void MasterServer::removeSession( MasterSession& session )
	{
		session.removeSelf();
	}

	MM_TS MasterSession* MasterServer::findServerFromFilter( const SearchFilter& sf )
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
				auto* session = sl->findFromFilter( sf );
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
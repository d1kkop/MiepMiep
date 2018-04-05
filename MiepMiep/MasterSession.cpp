#include "MasterSession.h"
#include "MasterServer.h"
#include "LinkStats.h"
#include "LinkState.h"
#include "Util.h"


namespace MiepMiep
{
	// ------ Custom Serialization ----------------------------------------------------------------------------

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
			m_Link->getSession()->forListeners( [&] ( ISessionListener* l )
			{
				// If empty, we become host.
				l->onNewHost( m_Link->session(), m_NewHost.get() );
			} );
		}

		sptr<const IAddress> m_NewHost;
	};


	// ------ RPC --------------------------------------------------------------------------------

	// Executed on client (on connection to master).
	MM_RPC( masterSessionConnectTo, u32, sptr<IAddress> )
	{
		RPC_BEGIN();
		u32 linkId = get<0>( tp );
		LOG( "Connecting with Id %d.", linkId );
		const sptr<IAddress>& toAddr = get<1>( tp );
		sptr<Link> newLink = nw.getOrAdd<LinkManager>()->add( s, SocketAddrPair( l.socket(), *toAddr ), linkId, false /*addHandler*/ );
		if ( newLink )
		{
			assert( s.hasLink( *newLink ) );
			newLink->getOrAdd<LinkState>()->connect( l.normalSession().metaData() );
		}
	}

	// Executed on client (on connection to master).
	MM_RPC( masterSessionNewHost, sptr<IAddress> )
	{
		RPC_BEGIN();
		const sptr<IAddress>& host = get<0>( tp );
		l.normalSession().updateHost( host );
		l.pushEvent<EventNewHost>( host );
	}


	// ------ MasterSession ----------------------------------------------------------------------------

	MasterSession::MasterSession( const sptr<Link>& host, const MasterSessionData& data, const MasterSessionList& sessionList ):
		SessionBase( host->m_Network ),
		m_SessionList( sessionList.ptr<MasterSessionList>() ),
		m_NewLinksCanJoin( true )
	{
		m_Host = host;
		m_MasterData = data;
	}

	MM_TS sptr<const IAddress> MasterSession::host() const
	{
		scoped_lock lk(m_DataMutex);
		auto shost = m_Host.lock();
		return shost ? shost->destination().to_ptr() : nullptr;
	}

	MM_TS bool MasterSession::addLink( const sptr<Link>& newLink )
	{
		scoped_lock lk( m_DataMutex );
		if ( !canJoinNoLock() )
			return false;
		if ( m_MasterData.m_IsP2p )
		{
			// Have all existing links connect to new link and new link connect to all existing links.
			auto addrCpy = newLink->destination().getCopy();
			for ( auto& l : m_Links )
			{
				auto sl = l.lock();
				if ( !sl ) continue;
				u32 linkId = Util::rand();
				LOG("Connect links on ID %d.", linkId);
				sl->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, addrCpy );
				newLink->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, sl->destination().getCopy() );
			}
		}
		else
		{
			auto shost = m_Host.lock();
			if ( shost )
			{
				// For client-server architecture, only have new client join to host.
				u32 linkId = Util::rand();
				shost->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, newLink->destination().getCopy() );
				newLink->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, shost->destination().getCopy() );
			}
		}
	#if _DEBUG
		for ( auto& l : m_Links )
		{
			if ( auto sl = l.lock() )
			{
				assert( *sl != *newLink );
			}
		}
	#endif
		m_Links.emplace_back( newLink );
		auto shost = m_Host.lock();
		if ( shost )
		{
			newLink->callRpc<masterSessionNewHost, sptr<IAddress>>( shost->destination().getCopy() );
		}
		return true;
	}

	MM_TS void MasterSession::removeLink( Link& link )
	{
		scoped_lock lk( m_DataMutex );

		// TODO use map instead? 
	#if _DEBUG
		auto lIt = std::find_if( begin( m_Links ), end( m_Links ), [&] ( auto& l ) { return *l.lock()==link; } );
		if ( lIt == m_Links.end() )
		{
			assert( false );
			LOGW( "Link %s not found in master session.", link.info() );
		}
	#endif

		// If is P2p and session already started, new links can no longer join if someone left because
		// we do not have all addresses anymore that might be in the (game)session.
		if ( m_MasterData.m_IsP2p && m_Started )
		{
			m_NewLinksCanJoin = false;
		}

		// If leaving link is host, determine new host.
		auto shost = m_Host.lock();
		if ( link == *shost )
		{
			u32 highestScore = 0;
			shost.reset();
			for ( auto& l : m_Links )
			{
				auto sl = l.lock();
				if ( *sl == link ) continue; // skip leaving host
				u32 score = sl->getOrAdd<LinkStats>()->hostScore();
				if ( score > highestScore )
				{
					highestScore = score;
					m_Host = sl;
				}
			}
			assert( (m_Links.empty() && !m_Host.lock()) || (!m_Links.empty() && m_Host.lock()) );
			shost = m_Host.lock();
			if ( shost )
			{
				link.network().callRpc<masterSessionNewHost, sptr<IAddress>>(
					shost->destination().getCopy(), &link.session(), &link,
					No_Local, No_Buffer, No_Relay, MM_RPC_CHANNEL, No_Trace );
			}
		}

		std::remove_if( begin( m_Links ), end( m_Links ), [&] ( auto l ) { return *l.lock()==link; } );
	}

	MM_TS void MasterSession::updateHost( const sptr<Link>& link )
	{
		scoped_lock lk( m_DataMutex);
		m_Host = link;
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
		auto& d = m_MasterData;
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
		return m_Host.lock() && m_NewLinksCanJoin && (!m_Started || m_MasterData.m_CanJoinAfterStart);
	}


	MM_TS const ILink& MasterSession::matchMaker() const
	{
		static sptr<Link> emptyLink;
		assert( false );
		return *emptyLink;
	}
}
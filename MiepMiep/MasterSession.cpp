#include "MasterSession.h"
#include "MasterServer.h"
#include "LinkStats.h"
#include "LinkState.h"
#include "Util.h"
#include "Endpoint.h"

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
		const sptr<IAddress>& toAddr = get<1>( tp );
		LOG( "Connecting to %s.", toAddr->toIpAndPort() );
		sptr<Link> newLink = nw.getOrAdd<LinkManager>()->getOrAdd ( &s, SocketAddrPair( l.socket(), *toAddr ), linkId, 
                                                                   false /*addHandler*/, true /* addToSession */, true, nullptr );
		if ( newLink )
		{
			assert( s.hasLink( *newLink ) );
			newLink->getOrAdd<LinkState>()->connect( l.normalSession().metaData() );
		}
	}

	// Executed on client (on new host in master session)
	MM_RPC( masterSessionNewHost, sptr<IAddress> )
	{
		RPC_BEGIN();
		sptr<IAddress> hostAddr = get<0>( tp );
		if ( sc<Endpoint&>(*hostAddr).isZero() )
		{
			// we are host
			hostAddr.reset(); 
		}
		l.normalSession().updateHost( hostAddr );
		l.pushEvent<EventNewHost>( hostAddr );
	}


	// ------ MasterSession ----------------------------------------------------------------------------

	MasterSession::MasterSession( const sptr<Link>& host, const MasterSessionData& data, const MasterSessionList& sessionList ):
		SessionBase( host->m_Network ),
		m_SessionList( sessionList.ptr<MasterSessionList>() ),
		m_SomeoneLeftTheSession( false )
	{
		m_Host = host;
		m_MasterData = data;
	}

	MM_TS sptr<const IAddress> MasterSession::host() const
	{
		rscoped_lock lk(m_DataMutex);
		auto shost = m_Host.lock();
		return shost ? shost->destination().to_ptr() : nullptr;
	}

	MM_TS bool MasterSession::addLink( const sptr<Link>& newLink )
	{
		rscoped_lock lk( m_DataMutex );
		if ( !canJoin() )
			return false;
		SessionBase::addLink( newLink );
		auto shost = m_Host.lock();
		assert( shost ); // Must always have a host.
		if ( *newLink == *shost )
		{
			// If destination becomes host, send empty address
			newLink->callRpc<masterSessionNewHost, sptr<IAddress>>( IAddress::createEmpty() );
		}
		else
		{
			newLink->callRpc<masterSessionNewHost, sptr<IAddress>>( shost->destination().getCopy() );
		}
		return true;
	}

	MM_TS void MasterSession::removeLink( Link& link )
	{
		rscoped_lock lk( m_DataMutex );
		auto lIt = std::find_if( begin( m_Links ), end( m_Links ), [&] ( auto& l ) { return *l.lock()==link; } );
		if ( lIt == m_Links.end() )
		{
			assert( false );
			LOGW( "Link %s not found in master session.", link.info() );
			return; // Return, nothing will change.
		}

		m_Links.erase( lIt );
		m_SomeoneLeftTheSession = true;

		// If leaving link is host, determine new host.
		auto shost = m_Host.lock();
		if ( link == *shost )
		{
			u32 highestScore = 0;
			shost.reset();
			// Try find host with best score.
			for ( auto& l : m_Links )
			{
				auto sl = l.lock();
				if ( !sl ) continue; // discard invalid links
				assert( !(*sl == link) ); // Removed from session already.
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
				// Send to all except the one that becomes the host, the new host.
				link.network().callRpc<masterSessionNewHost, sptr<IAddress>>(
					shost->destination().getCopy(), &link.session(), shost.get(), /* excl host */
					No_Local, No_Buffer, No_Relay, MM_RPC_CHANNEL, No_Trace );
				// To host self, send empty address
				shost->callRpc<masterSessionNewHost, sptr<IAddress>>(
					IAddress::createEmpty(), 
					No_Local, No_Relay, MM_RPC_CHANNEL, No_Trace );
			}
			else
			{
				// Could not determine new host, this means that session is empty. Remove session.
				removeSelf();
			}
		}
	}

	MM_TS void MasterSession::updateHost( const sptr<Link>& link )
	{
		rscoped_lock lk( m_DataMutex );
		m_Host = link;
	}

	MM_TS void MasterSession::removeSelf()
	{
		rscoped_lock lk( m_DataMutex );
		// First clean links
		for ( auto it = m_Links.begin(); it != m_Links.end(); )
		{
			auto sl = (*it).lock();
			if ( !sl )
			{
				it = m_Links.erase( it );
			}
			else it++;
		}
		// Now assert that links are all gone.
		assert( m_Links.empty() );
		if ( auto sl = m_SessionList.lock() )
		{
			sl->removeSession( m_SessionListIt );
			m_SessionList.reset();
		}
	}

	MM_TS bool MasterSession::operator==( const SearchFilter& sf ) const
	{
		rscoped_lock lk( m_DataMutex );
		auto& d = m_MasterData;
		return
			canJoin() &&
			d.m_Name == sf.m_Name &&
			d.m_Type == sf.m_Type &&
			(!d.m_IsPrivate || sf.m_FindPrivate) &&
			((d.m_IsP2p && sf.m_FindP2p) || (!d.m_IsP2p && sf.m_FindClientServer)) &&
			((u32)m_Links.size() >= sf.m_MinPlayers && d.m_MaxClients <= sf.m_MaxPlayers) &&
			(d.m_Rating >= sf.m_MinRating && d.m_Rating <= sf.m_MaxRating);
	}

	MM_TS bool MasterSession::canJoin() const
	{
		rscoped_lock lk( m_DataMutex );
		return (!m_MasterData.m_IsP2p || !m_SomeoneLeftTheSession) && (!m_Started || m_MasterData.m_CanJoinAfterStart);
	}


	MM_TS sptr<ILink> MasterSession::matchMaker() const
	{
		return nullptr;
	}

	MM_TS void MasterSession::sendConnectRequests( Link& newLink )
	{
		rscoped_lock lk(m_DataMutex);
		if ( m_MasterData.m_IsP2p )
		{
			// Have all existing links connect to new link and new link connect to all existing links.
			auto addrCpy = newLink.destination().getCopy();
			for ( auto& l : m_Links )
			{
				auto sl = l.lock();
				if ( !sl ) continue;
				u32 linkId = Util::rand();
				LOG( "Connect links on ID %d.", linkId );
				sl->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, addrCpy );
				newLink.callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, sl->destination().getCopy() );
			}
		}
		else
		{
			auto shost = m_Host.lock();
			assert( shost );
			// For client-server architecture, only have new client join to host.
			u32 linkId = Util::rand();
			//shost->callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, newLink.destination().getCopy() );
			newLink.callRpc<masterSessionConnectTo, u32, sptr<IAddress>>( linkId, shost->destination().getCopy() );
		}
	}

}
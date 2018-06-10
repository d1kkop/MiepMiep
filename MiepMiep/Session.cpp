#include "Session.h"
#include "Link.h"
#include "MasterServer.h"
#include "ReliableSend.h"
#include <algorithm>


namespace MiepMiep
{
	// --------- ISession ----------------------------------------------------------------------------------------------

	MM_TS sptr<ISession> ISession::to_ptr()
	{
		return sc<Session&>( *this ).ptr<Session>();
	}

	MM_TS sptr<const ISession> ISession::to_ptr() const
	{
		return sc<const Session&>( *this ).ptr<const Session>();
	}


	// --------- Session ----------------------------------------------------------------------------------------------

	Session::Session( Network& network, const MetaData& md ):
		SessionBase( network ),
		m_MetaData(md)
	{
	}

	MM_TSC void Session::setMasterLink( const sptr<Link>& link )
	{
		// Supposed to be set only at construction, no need for lock.
		assert(link);
		m_MasterLink = link;
	}

	MM_TS sptr<const IAddress> Session::host() const
	{
		rscoped_lock lk( m_DataMutex );
		return m_Host;
	}

	MM_TS sptr<ILink> Session::matchMaker() const
	{
		rscoped_lock lk( m_DataMutex );
		return m_MasterLink.lock();
	}

	MM_TS void Session::removeLink( Link& link )
	{
		removeLink( link.to_ptr() );
	}

	MM_TS void Session::updateHost( const sptr<const IAddress>& hostAddr )
	{
		rscoped_lock lk( m_DataMutex );
		m_Host = hostAddr;
	}

	MM_TS void Session::removeLink( const sptr<const Link>& link )
	{
		rscoped_lock lk( m_DataMutex );
		for ( auto it = begin(m_Links); it !=end(m_Links); )
		{
			auto sl = it->lock();
			if ( !sl || *sl == *link )
			{
				it = m_Links.erase(it);
			}
			else it ++;
		}
		//std::remove_if( m_Links.begin(), m_Links.end(), [&] ( auto& l ) 
		//{ 
		//	auto sl = l.lock();
		//	bool bRemoved = !sl || (*sl == *link);
		//	return bRemoved;
		//} );
	}

	MM_TS bool Session::disconnect()
	{
		forLink( nullptr, [] ( Link& link )
		{
			link.disconnect( No_Kick, No_Receive, Remove_Link );
		});
		return true; // TODO make more sense on return
	}


	MM_TO_PTR_IMP( Session )

}
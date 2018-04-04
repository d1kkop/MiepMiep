#include "Session.h"
#include "Link.h"
#include "MasterServer.h"
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
		assert(!m_MasterLink && link);
		m_MasterLink = link;
	}

	MM_TS sptr<const IAddress> Session::host() const
	{
		scoped_lock lk(m_DataMutex);
		return m_Host;
	}

	MM_TS const ILink& Session::matchMaker() const
	{
		scoped_lock lk(m_DataMutex);
		return *m_MasterLink;
	}

	MM_TS bool Session::addLink( const sptr<Link>& link )
	{
		scoped_lock lk(m_LinksMutex);
	#if _DEBUG
		LOG( "Added link %s to session %d.", link->info(), m_Id );
		for ( auto& l : m_Links )
		{
			assert( !(*l == *link) );
		}
	#endif
		m_Links.emplace_back( link );
		return true;
	}

	MM_TS void Session::removeLink( Link& link )
	{
		removeLink( link.to_ptr() );
	}

	MM_TS void Session::updateHost( const sptr<const IAddress>& hostAddr )
	{
		scoped_lock lk( m_DataMutex );
		m_Host = hostAddr;
	}

	MM_TS void Session::removeLink( const sptr<const Link>& link )
	{
		scoped_lock lk( m_LinksMutex );
		std::remove( m_Links.begin(), m_Links.end(), link );
	}

	MM_TS bool Session::disconnect()
	{
		forLink( nullptr, [] ( Link& link )
		{
			link.disconnect( false, true );
		});
		return true; // TODO make more sense on return
	}


	MM_TO_PTR_IMP( Session )

}
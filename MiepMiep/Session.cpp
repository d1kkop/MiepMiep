#include "Session.h"
#include "Link.h"
#include "MasterServer.h"
#include <algorithm>


namespace MiepMiep
{
	Session::Session( Network& network, const sptr<Link>& masterLink, const MetaData& md ):
		ParentNetwork( network ),
		m_MasterLink(masterLink),
		m_MetaData(md)
	{
	}

	MM_TS const ILink& Session::matchMaker() const
	{
		scoped_spinlock lk(m_DataMutex);
		return *m_MasterLink;
	}

	MM_TS const IAddress* Session::host() const
	{
		scoped_spinlock lk(m_DataMutex);
		return m_Host.get();
	}

	MM_TS void Session::addLink( const sptr<Link>& link )
	{
		scoped_lock lk(m_LinksMutex);
		m_Links.emplace_back( link );
	}

	MM_TS void Session::removeLink( const sptr<const Link>& link )
	{
		scoped_lock lk( m_LinksMutex );
		std::remove(m_Links.begin(), m_Links.end(), link);
	}

	MM_TS void Session::removeLink( const Link& link )
	{
		removeLink( link.to_ptr() );
	}

	MM_TS bool Session::hasLink( const Link& link ) const
	{
		scoped_lock lk( m_LinksMutex );
		for ( auto& l : m_Links )
		{
			if ( *l == link ) return true;
		}
		return false;
	}

	MM_TS void Session::forLink( const Link* exclude, const std::function<void( Link& )>& cb ) const
	{
		scoped_lock lk( m_LinksMutex );
		for ( auto& l : m_Links )
		{
			if ( l.get() == exclude ) continue;
			cb ( *l );
		}
	}

	MM_TS bool Session::disconnect()
	{
		forLink( nullptr, [] ( Link& link )
		{
			link.disconnect( false, true );
		});
		return true; // TODO make more sense on return
	}

	MM_TSC const MasterSessionData& Session::msd() const
	{
		return *m_MasterData;
	}


	MM_TO_PTR_IMP( Session )

}
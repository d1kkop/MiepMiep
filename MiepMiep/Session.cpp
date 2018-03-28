#include "Session.h"
#include "Link.h"
#include <algorithm>


namespace MiepMiep
{
	Session::Session( Network& network, const sptr<Link>& masterLink, const string& pw, const MetaData& md ):
		ParentNetwork( network ),
		m_MasterLink(masterLink),
		m_Pw(pw),
		m_MetaData(md)
	{
	}

	MM_TS void Session::addLink( const sptr<Link>& link )
	{
		scoped_lock lk(m_LinksMutex);
		m_Links.emplace_back( link );
	}

	MM_TS void Session::removeLink( const sptr<Link>& link )
	{
		scoped_lock lk( m_LinksMutex );
		std::remove(m_Links.begin(), m_Links.end(), link);
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

	MM_TO_PTR_IMP( Session )

}
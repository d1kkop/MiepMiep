#include "Session.h"
#include "Link.h"


namespace MiepMiep
{

	Session::Session( const sptr<Link>& masterLink, const string& pw, const MetaData& md ):
		m_MasterLink(masterLink),
		m_Pw(pw),
		m_MetaData(md)
	{
	}

	MM_TS void Session::addLink( const sptr<Link>& link )
	{

	}

	MM_TS void Session::removeLink( const sptr<Link>& link )
	{

	}

}
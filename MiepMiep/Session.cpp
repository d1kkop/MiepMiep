#include "Session.h"
#include "Link.h"
#include "MasterSessionManager.h"
#include "ReliableSend.h"
#include <algorithm>


namespace MiepMiep
{
	// --------- ISession ----------------------------------------------------------------------------------------------

	sptr<ISession> ISession::to_ptr()
	{
		return sc<Session&>( *this ).ptr<Session>();
	}

	sptr<const ISession> ISession::to_ptr() const
	{
		return sc<const Session&>( *this ).ptr<const Session>();
	}


	// --------- Session ----------------------------------------------------------------------------------------------

	Session::Session( Network& network, const MetaData& md ):
		SessionBase( network ),
		m_MetaData(md)
	{
	}

	void Session::setMasterLink( const sptr<Link>& link )
	{
		assert(link);
		m_MasterLink = link;
	}

	sptr<const IAddress> Session::host() const
	{
		return m_Host;
	}

	sptr<ILink> Session::matchMaker() const
	{
		return m_MasterLink.lock();
	}

	void Session::removeLink( Link& link )
	{
		removeLink( link.to_ptr() );
	}

	void Session::updateHost( const sptr<const IAddress>& hostAddr )
	{
		m_Host = hostAddr;
	}

	void Session::removeLink( const sptr<const Link>& link )
	{
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

	bool Session::disconnect()
	{
		forLink( nullptr, [] ( Link& link )
		{
			link.disconnect( No_Kick, No_Receive, Remove_Link );
		});
		return true; // TODO make more sense on return
	}


	MM_TO_PTR_IMP( Session )

}
#include "LinkManager.h"
#include "Util.h"


namespace MiepMiep
{

	LinkManager::LinkManager(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS sptr<Link> LinkManager::getOrAdd(const IEndpoint& etp, u32* id, const Listener* originator, bool* wasAdded)
	{
		scoped_lock lk(m_LinksMapMutex);

		auto etpPtr = etp.to_ptr();
		if ( m_Links.count( etpPtr ) != 0 ) 
		{
			if (wasAdded) *wasAdded = false;
			return m_Links[etpPtr];
		}

		// create fail
		sptr<Link> link = Link::create(m_Network, etp, id, originator);
		if (!link)  
		{
			if (wasAdded) *wasAdded = false;
			return nullptr;
		}

		// insert
		m_Links[etpPtr] = link;
		m_LinksAsArray.emplace_back( link );
		if ( wasAdded ) *wasAdded = true;

		return link;
	}

	MM_TS sptr<Link> LinkManager::getLink(const IEndpoint& etp)
	{
		scoped_lock lk(m_LinksMapMutex);
		auto etpPtr = etp.to_ptr();
		if ( m_Links.count( etpPtr ) != 0 ) 
			return m_Links[etpPtr];
		return nullptr;
	}

	MM_TS void LinkManager::forEachLink(const std::function<void(Link&)>& cb, u32 clusterSize)
	{
		if ( 0 == clusterSize )
		{
			scoped_lock lk(m_LinksMapMutex);
			for ( auto& link : m_LinksAsArray )
				cb( *link );
		}
		else
		{
			auto lm = this->ptr<LinkManager>();
			Util::cluster( m_LinksAsArray.size(), clusterSize, [&, lm]( auto s, auto e )
			{
				while ( s < e )
				{
					cb( *lm->m_LinksAsArray[s] );
					++s;
				}
			});
		}
	}

	MM_TS bool LinkManager::forLink(const IEndpoint* specific, bool exclude, const std::function<void(Link&)>& cb)
	{
		bool wasSentAtLeastOnce = false;
		scoped_lock lk(m_LinksMapMutex);
		if ( specific ) 
		{
			if ( !exclude )
			{
				auto linkIt = m_Links.find( specific->to_ptr() );
				if ( linkIt != m_Links.end() ) 
				{
					cb ( *linkIt->second );
					wasSentAtLeastOnce = true;
				}
			}
			else
			{
				for ( auto& link : m_LinksAsArray )
				{
					if ( link->remoteEtp() == *specific ) 
						continue; // skip this one
					cb ( *link );
				}
				wasSentAtLeastOnce = m_Links.size() > 1 || ((!m_Links.empty()) && (*specific != *m_Links.begin()->first));
			}
		}
		else
		{
			for ( auto& link : m_LinksAsArray )
				cb ( *link );
			wasSentAtLeastOnce = !m_LinksAsArray.empty();
		}
		return wasSentAtLeastOnce;
	}

}
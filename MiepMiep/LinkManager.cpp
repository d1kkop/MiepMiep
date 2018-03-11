#include "LinkManager.h"
#include "Util.h"


namespace MiepMiep
{

	LinkManager::LinkManager(Network& network):
		ParentNetwork(network)
	{
	}

	MM_TS sptr<Link> LinkManager::addLink(const IEndpoint& etp)
	{
		auto etpCpy = etp.getCopy();
		sptr<Link> link = Link::create(m_Network, etp);
		if (!link)  return nullptr;

		scoped_lock lk(m_LinksMapMutex);
		if ( m_Links.count( etpCpy ) != 0 ) return nullptr;
		m_Links[etpCpy] = link;
		m_LinksAsArray.emplace_back( link );

		return link;
	}

	MM_TS sptr<MiepMiep::Link> LinkManager::getLink(const IEndpoint& etp)
	{
		auto etpCpy = etp.getCopy();
		scoped_lock lk(m_LinksMapMutex);
		if ( m_Links.count( etpCpy ) != 0 ) return m_Links[etpCpy];
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
				auto linkIt = m_Links.find( specific->getCopy() );
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
#include "LinkManager.h"


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

		return link;
	}

	MM_TS sptr<MiepMiep::Link> LinkManager::getLink(const IEndpoint& etp)
	{
		auto etpCpy = etp.getCopy();
		scoped_lock lk(m_LinksMapMutex);
		if ( m_Links.count( etpCpy ) != 0 ) return m_Links[etpCpy];
		return nullptr;
	}

	MM_TS void LinkManager::forEachLink(const std::function<void(Link&)>& cb)
	{
		scoped_lock lk(m_LinksMapMutex);
		for ( auto& kvp : m_Links )
			cb( *kvp.second );
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
				for ( auto& kvp : m_Links )
				{
					if ( *kvp.first == *specific ) 
						continue; // skip this one
					cb ( *kvp.second );
				}
				wasSentAtLeastOnce = m_Links.size() > 1 || ((!m_Links.empty()) && (*specific != *m_Links.begin()->first));
			}
		}
		else
		{
			for ( auto& kvp : m_Links )
				cb ( *kvp.second );
			wasSentAtLeastOnce = !m_Links.empty();
		}
		return wasSentAtLeastOnce;
	}

}
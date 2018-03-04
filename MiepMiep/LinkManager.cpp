#include "LinkManager.h"


namespace MiepMiep
{

	LinkManager::LinkManager(Network& network):
		Parent(network)
	{

	}

	MM_TS sptr<Link> LinkManager::addLink(const IEndpoint& etp)
	{
		auto etpCpy = etp.getCopy();
		sptr<Link> link = Link::create(m_Parent, etp);
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

}
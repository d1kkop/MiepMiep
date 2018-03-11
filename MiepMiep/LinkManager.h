#pragma once

#include "Link.h"
#include "Component.h"
#include "Network.h"


namespace MiepMiep
{
	class LinkManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		LinkManager(Network& network);

		MM_TS sptr<Link> addLink( const IEndpoint& etp );
		MM_TS sptr<Link> getLink( const IEndpoint& etp );
		MM_TS void forEachLink( const std::function<void (Link&)>& cb, u32 clusterSize=0 );
		MM_TS bool forLink( const IEndpoint* specific, bool exclude, const std::function<void (Link&)>& cb );

		// IComponent
		static EComponentType compType() { return EComponentType::LinkManager; }

	private:
		mutex m_LinksMapMutex;
		map<sptr<IEndpoint>, sptr<Link>, IEndpoint::stl_less> m_Links;
		vector<sptr<Link>> m_LinksAsArray;
	};
}
#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class Link;
	class Listener;


	class LinkManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		LinkManager(Network& network);

		MM_TS sptr<Link> getOrAdd( const IAddress& etp, u32* id, const Listener* originator, bool* wasAdded=nullptr );
		MM_TS sptr<Link> getLink( const IAddress& etp );
		MM_TS void forEachLink( const std::function<void (Link&)>& cb, u32 clusterSize=0 );
		MM_TS bool forLink( const IAddress* specific, bool exclude, const std::function<void (Link&)>& cb );

		// IComponent
		static EComponentType compType() { return EComponentType::LinkManager; }

	private:
		mutex m_LinksMapMutex;
		map<sptr<const IAddress>, sptr<Link>, IEndpoint_less> m_Links;
		vector<sptr<Link>> m_LinksAsArray;
	};
}
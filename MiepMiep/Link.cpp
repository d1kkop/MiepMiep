#include "Link.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{

	Link::Link(Network& network):
		ParentNetwork(network)
	{
	}

	sptr<Link> Link::create(Network& network, const IEndpoint& other)
	{
		auto link = sptr<Link>(reserve<Link, Network&>(MM_FL, network));
		link->m_RemoteEtp = other.getCopy();
		link->m_Id = rand();
		return link;
	}

	MM_TS class BinSerializer& Link::beginSend()
	{
		auto& bs = PerThreadDataProvider::getSerializer();
		bs.write( m_Id );
		return bs;
	}

	MM_TS void Link::createGroup(const string& typeName, const BinSerializer& initData)
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionLink>()->addNewPendingGroup( varVec, typeName, initData, nullptr ); // makes copy
		varVec.clear();
	}

	MM_TS void Link::destroyGroup(u32 id)
	{
		
	}
}
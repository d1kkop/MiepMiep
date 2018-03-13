#include "GroupCollection.h"
#include "Link.h"
#include "Network.h"
#include "Variables.h"
#include "LinkManager.h"
#include "ReliableSend.h"
#include "JobSystem.h"
#include "PerThreadDataProvider.h"
#include "Group.h"
#include "BinSerializer.h"
#include "Platform.h"


namespace MiepMiep
{
	// ------- RPC ------------------------------------------------------------------------------------------

	MM_RPC(createGroup, string, u32, BinSerializer)
	{
		/* INetwork& network */
		/* const IEndpoint* etp */
		assert( etp != nullptr );
		Network& nw = static_cast<Network&>(network);
		nw.createRemoteGroup( get<0>(tp), get<1>(tp), get<2>(tp), *etp );
	}


	// ------- GroupCollection ------------------------------------------------------------------------------------------

	GroupCollection::GroupCollection()
	{
	}

	MM_TS void GroupCollection::addNewPendingGroup(vector<NetVariable*>& vars, const string& typeName, const BinSerializer& initData, IDeliveryTrace* trace)
	{
		sptr<Group> g = reserve_sp<Group, GroupCollection&, vector<NetVariable*>&, const string&, const BinSerializer&, EVarControl>
			(
				MM_FL, *this, vars, typeName, initData, EVarControl::Full
			);
		scoped_lock lk(m_GroupLock);
		m_PendingGroups.emplace_back( g );
		auto gc = ptr<GroupCollection>();
		network().get<JobSystem>()->addJob([&, gc]() { 
			tryProcessPendingGroups();
		});
	}

	MM_TS void GroupCollection::tryProcessPendingGroups()
	{
		scoped_lock lk(m_GroupLock);
		while ( !m_PendingGroups.empty() && !m_IdPool.empty() )
		{
			u32 id = m_IdPool.back();
			m_IdPool.pop_back();
			sptr<Group> group = m_PendingGroups.front();
			m_PendingGroups.pop_back();
			group->setId( id );
			__CHECKED( m_Groups.count( id ) != 0 );
			m_Groups[id] = group;
			msgGroupCreate( group->typeName(), group->id(), group->initData() );
		}
	}

	MM_TS sptr<Group> GroupCollection::findGroup(u32 netId) const
	{
		scoped_lock lk(m_GroupLock);
		auto gIt = m_Groups.find( netId );
		if ( gIt != m_Groups.end() )
			return gIt->second;
		return nullptr;
	}

	// ------- GroupCollectionLink ------------------------------------------------------------------------------------------


	Link* GroupCollectionLink::link() const
	{
		return &m_Link;
	}

	Network& GroupCollectionLink::network() const
	{
		return link()->m_Network;
	}

	void GroupCollectionLink::msgGroupCreate(const string& typeName, u32 groupId, const BinSerializer& initData)
	{
		// Only send group create on this link.
		m_Link.callRpc<createGroup, string, u32, BinSerializer>
			(
				typeName, groupId, initData, false, false,
				MM_VG_CHANNEL, nullptr
			);
	}


	// ------- GroupCollectionNetwork ------------------------------------------------------------------------------------------


	Link* GroupCollectionNetwork::link() const
	{
		return nullptr;
	}

	Network& GroupCollectionNetwork::network() const
	{
		return m_Network;
	}

	MM_TS void GroupCollectionNetwork::msgGroupCreate( const string& typeName, u32 groupId, const BinSerializer& initData )
	{
		// Send group create to all.
		network().callRpc2<createGroup, string, u32, BinSerializer>
			(
				typeName, groupId, initData, false, 
				nullptr, false, true, true, true,
				MM_VG_CHANNEL, nullptr
			);
	}

}
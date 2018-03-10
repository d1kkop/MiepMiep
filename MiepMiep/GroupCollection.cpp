#include "GroupCollection.h"
#include "LinkManager.h"
#include "ReliableSend.h"
#include "JobSystem.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
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

	void GroupCollectionLink::msgGroupCreate( const string& typeName, u32 groupId, const BinSerializer& initData )
	{
		// Only send group create on this link.
		inetwork().callRpc<createGroup, string, u32, BinSerializer>
			(
				typeName, groupId, initData, false,
				&link()->remoteEtp(), false, false, false, 
				MM_VG_CHANNEL, nullptr
			);
	}


	// ------- GroupCollectionNetwork ------------------------------------------------------------------------------------------

	MM_TS void GroupCollectionNetwork::msgGroupCreate( const string& typeName, u32 groupId, const BinSerializer& initData )
	{
		// Send group create to all.
		inetwork().callRpc<createGroup, string, u32, BinSerializer>
			(
				typeName, groupId, initData, false, 
				nullptr, false, true, true, 
				MM_VG_CHANNEL, nullptr
			);
	}

}
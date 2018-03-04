#include "GroupCollection.h"
#include "LinkManager.h"
#include "ReliableSend.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
	// ------- GroupCollection ------------------------------------------------------------------------------------------

	GroupCollection::GroupCollection()
	{
	}

	MM_TS Group* GroupCollection::addNewPendingGroup(vector<NetVariable*>& vars, u16 groupType)
	{
		Group* g = reserve<Group, GroupCollection&, vector<NetVariable*>&, u16>(MM_FL, *this, vars, groupType);
		scoped_lock lk(m_GroupLock);
		m_PendingGroups.emplace_back( g );
		return g;
	}

	void GroupCollection::tryProcessPendingGroups()
	{
		scoped_lock lk(m_GroupLock);
		while ( !m_PendingGroups.empty() && !m_IdPool.empty() )
		{
			u32 id = m_IdPool.back();
			m_IdPool.pop_back();
			sptr<Group> group = m_PendingGroups.front();
			m_PendingGroups.pop_back();
			// init group now that all required info is available
			group->setId( id );
			group->setOwner( nullptr ); // we own it
			group->setControl( EVarControl::Full );
			__CHECKED( m_Groups.count( id ) != 0 );
			m_Groups[id] = group;
			msgGroupCreate( group->typeId(), group->id(), true );
		}
	}


	// ------- GroupCollectionLink ------------------------------------------------------------------------------------------

	void GroupCollectionLink::msgGroupCreate( u16 typeId, u32 groupId, bool write )
	{
		auto& bs = PerThreadDataProvider::getSerializer();
		__CHECKED( bs.readOrWrite(typeId, write) );
		__CHECKED( bs.readOrWrite(groupId, write) );
		if ( write )
		{
			//link()->getOrAdd<ReliableSend>()->
		}
		else
		{
			network().getOrAdd<GroupCollectionNetwork>()->createFromRemote( typeId, groupId );
		}
	}


	// ------- GroupCollectionNetwork ------------------------------------------------------------------------------------------

	MM_TS void GroupCollectionNetwork::msgGroupCreate( u16 typeId, u32 groupId, bool write )
	{
		network().getOrAdd<LinkManager>()->forEachLink( [=](Link& link)
		{
			link.getOrAdd<GroupCollectionLink>()->msgGroupCreate( typeId, groupId, write );
		});
	}

	void GroupCollectionNetwork::createFromRemote(u16 typeId, u32 groupId)
	{

	}


}
#include "GroupCollection.h"
#include "Link.h"
#include "Network.h"
#include "JobSystem.h"
#include "Group.h"


namespace MiepMiep
{
	// ------- RPC ------------------------------------------------------------------------------------------

	// [ typeName, id, initData ]
	MM_RPC(createGroup, string, u32, BinSerializer)
	{
		RPC_BEGIN();
		nw.createRemoteGroup( get<0>(tp), get<1>(tp), get<2>(tp), l.destination() );
	}


	// ------- GroupCollection ------------------------------------------------------------------------------------------

	GroupCollection::GroupCollection()
	{
	}

	MM_TS void GroupCollection::addNewPendingGroup( const Session& session, vector<NetVariable*>& vars, const string& typeName,
												    const BinSerializer& initData, IDeliveryTrace* trace)
	{
		// Do not allow creating a group when no variables are specified.
		// First of all, without any variables there is nothing to sync.
		// Second, variables hold the ownership, and so it cannot be determined when creating a group on the remote.
		if ( vars.empty() )
		{
			LOGW( "No variable group created because no variables were specified. Call discarded." );
			return;
		}
		sptr<Group> g = reserve_sp<Group, GroupCollection&, const Session&, vector<NetVariable*>&, const string&, const BinSerializer&, EVarControl>
			(
				MM_FL, *this, session, vars, typeName, initData, EVarControl::Full
			);
		scoped_lock lk(m_GroupLock);
		m_PendingGroups.emplace_back( g );
		network().get<JobSystem>()->addJob([gc = move(ptr<GroupCollection>())]
		{ 
			gc->tryProcessPendingGroups();
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
			msgGroupCreate( &group->session(), group->typeName(), group->id(), group->initData() );
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

	void GroupCollectionLink::msgGroupCreate(const Session* session, const string& typeName, u32 groupId, const BinSerializer& initData)
	{
		// Only send group create on this link.
		assert(!session);
		m_Link.callRpc<createGroup, string, u32, BinSerializer>
			(
				typeName, move( groupId ), initData, false, false,
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

	MM_TS void GroupCollectionNetwork::msgGroupCreate( const Session* session, const string& typeName, u32 groupId, const BinSerializer& initData )
	{
		// Send group create to all.
		assert(session);
		network().callRpc2<createGroup, string, u32, BinSerializer>
			(
				typeName, groupId, initData, session, nullptr,
				false, true, true, true,
				MM_VG_CHANNEL, nullptr
			);
	}

}
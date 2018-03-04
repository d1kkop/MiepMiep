#pragma once

#include "Group.h"
#include "Memory.h"
#include "Component.h"
#include "Link.h"
#include "Network.h"
#include "Rpc.h"


namespace MiepMiep
{
	class GroupCollection: public IComponent, public ITraceable
	{
	public:
		GroupCollection();

		MM_TS Group* addNewPendingGroup(vector<NetVariable*>& vars, u16 groupType);

		void tryProcessPendingGroups();

		virtual Link* link() const = 0;
		virtual Network& network() const = 0;
		virtual void msgGroupCreate( u16 typeId, u32 groupId, bool write ) = 0;

	protected:
		vector<u32> m_IdPool;
		mutex m_GroupLock;
		deque<sptr<Group>>			m_PendingGroups;
		map<u32, sptr<Group>>		m_Groups;
	};


	class GroupCollectionLink: public Parent<Link>, public GroupCollection
	{
	public:
		GroupCollectionLink(Link& link):
			Parent(link) { }

		static EComponentType compType() { return EComponentType::GroupCollectionLink; }

		Link* link() const override { return &m_Parent; }
		Network& network() const override { return m_Parent.m_Parent; }
		void msgGroupCreate( u16 typeId, u32 groupId, bool write ) override;
	};


	class GroupCollectionNetwork: public Parent<Network>, public GroupCollection
	{
	public:
		GroupCollectionNetwork(Network& network):
			Parent(network) { }

		static EComponentType compType() { return EComponentType::GroupCollectionNetwork; }

		Link* link() const override { return nullptr; }
		Network& network() const override { return m_Parent; }
		void msgGroupCreate( u16 typeId, u32 groupId, bool write ) override;
		void createFromRemote( u16 typeId, u32 groupId );
	};


	MM_RPC(createGroup, u16, u32)
	{
		/* INetwork& network */
		/* const IEndpoint* etp */
		i16 type  = std::get<0>(tp);
		i32 netId = std::get<1>(tp);
	}
}




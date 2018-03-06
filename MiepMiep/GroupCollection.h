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

		MM_TS void addNewPendingGroup(vector<NetVariable*>& vars, const string& typeName, const BinSerializer& initData, IDeliveryTrace* trace);
		MM_TS void tryProcessPendingGroups();
		MM_TS sptr<Group> findGroup( u32 netId ) const;

		virtual Link* link() const = 0;
		virtual Network& network() const = 0;
		INetwork& inetwork() const { return static_cast<INetwork&>(network()); }
		virtual void msgGroupCreate( const string& typeName, u32 groupId, const BinSerializer& initData ) = 0;

	protected:
		vector<u32>	  m_IdPool;
		mutable mutex m_GroupLock;
		deque<sptr<Group>>			m_PendingGroups;
		map<u32, sptr<Group>>		m_Groups;
	};


	class GroupCollectionLink: public ParentLink, public GroupCollection
	{
	public:
		GroupCollectionLink(Link& link):
			ParentLink(link) { }

		static EComponentType compType() { return EComponentType::GroupCollectionLink; }

		Link* link() const override { return &m_Link; }
		Network& network() const override { return link()->m_Network; }
		void msgGroupCreate( const string& typeName, u32 groupId, const BinSerializer& initData ) override;
	};


	class GroupCollectionNetwork: public ParentNetwork, public GroupCollection
	{
	public:
		GroupCollectionNetwork(Network& network):
			ParentNetwork(network) { }

		static EComponentType compType() { return EComponentType::GroupCollectionNetwork; }

		Link* link() const override { return nullptr; }
		Network& network() const override { return m_Network; }
		void msgGroupCreate( const string& typeName, u32 groupId, const BinSerializer& initData ) override;
	};


	MM_RPC(createGroup, string, u32, BinSerializer)
	{
		/* INetwork& network */
		/* const IEndpoint* etp */
		assert( etp != nullptr );
		Network& nw = static_cast<Network&>(network);
		nw.createRemoteGroup( get<0>(tp), get<1>(tp), get<2>(tp), *etp );
	}
}




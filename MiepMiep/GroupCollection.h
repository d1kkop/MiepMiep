#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"
#include "ParentNetwork.h"


namespace MiepMiep
{
	class Link;
	class Group;
	class Network;
	class INetwork;
	class Session;
	class NetVariable;


	class GroupCollection : public IComponent, public ITraceable
	{
	public:
		GroupCollection();

		void addNewPendingGroup( const Session& session, vector<NetVariable*>& vars, const string& typeName, const BinSerializer& initData, IDeliveryTrace* trace );
		void tryProcessPendingGroups();
		sptr<Group> findGroup( u32 netId ) const;

		virtual Link* link() const = 0;
		virtual Network& network() const = 0;
		virtual void msgGroupCreate( const Session* session, const string& typeName, u32 groupId, const BinSerializer& initData ) = 0;

	protected:
		vector<u32>	  m_IdPool;
		deque<sptr<Group>>		m_PendingGroups;
		map<u32, sptr<Group>>	m_Groups;
	};


	class GroupCollectionLink : public ParentLink, public GroupCollection
	{
	public:
		GroupCollectionLink( Link& link ) :
			ParentLink( link ) { }

		static EComponentType compType() { return EComponentType::GroupCollectionLink; }

		Link* link() const override;
		Network& network() const override;
		void msgGroupCreate( const Session* session, const string& typeName, u32 groupId, const BinSerializer& initData ) override;
	};


	class GroupCollectionNetwork : public ParentNetwork, public GroupCollection
	{
	public:
		GroupCollectionNetwork( Network& network ) :
			ParentNetwork( network ) { }

		static EComponentType compType() { return EComponentType::GroupCollectionNetwork; }

		Link* link() const override;
		Network& network() const override;
		void msgGroupCreate( const Session* session, const string& typeName, u32 groupId, const BinSerializer& initData ) override;
	};
}




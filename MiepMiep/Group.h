#pragma once

#include "BinSerializer.h"
#include "Memory.h"
#include <vector>
#include <cassert>
using namespace std;


namespace MiepMiep
{
	class Network;
	class Session;
	class NetVar;
	class NetVariable;
	class GroupCollection;


	class Group: public ITraceable
	{
	public:
		Group(GroupCollection& groupCollection, const Session& session, vector<NetVariable*>& vars, 
			  const string& typeName, const BinSerializer& initData, EVarControl initControlType);
		~Group() override;

		// Post set after Id is available. 
		// Once set from job in job system. No need for lock.
		void setId(u32 _id)							{ assert(m_Id==-1); m_Id = _id; }

		// Getters on group basis.
		// Set initially through constructor, no need for lock.
		u32 id() const								{ return m_Id; }
		const string& typeName() const				{ return m_TypeName; }
		const BinSerializer& initData() const		{ return m_InitData; }

		// Flag dirty when any of the variables inside the group get written/changed.
		void markChanged();
		void unGroup();
		void setNewOwnership( byte varIdx, const IAddress* owner );
		bool wasUngrouped() const;

		NetVar* getUserVar( byte idx ) const;

		Network& network() const;
		const Session& session() const;

		MM_TO_PTR(Group)
		
	private:
		GroupCollection& m_GroupCollection;
		u32 m_Id;
		string m_TypeName;
		bool m_WasUngrouped;
		volatile bool m_Changed;
		bool m_RemotelyCreated;
		vector<NetVariable*> m_Variables;
		BinSerializer m_InitData;
		sptr<const Session> m_Session;
	};
}

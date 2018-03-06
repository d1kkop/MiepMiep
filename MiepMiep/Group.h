#pragma once

#include "NetVariable.h"
#include "Memory.h"
#include <vector>
#include <cassert>
using namespace std;


namespace MiepMiep
{
	class GroupCollection;


	class Group: public ITraceable
	{
	public:
		Group(GroupCollection& groupCollection, vector<NetVariable*>& vars, const string& typeName, const BinSerializer& initData);
		~Group() override;

		// Post set after Id, control and owner are know.
		void setId(u32 _id)							{ assert(m_Id==-1); m_Id = _id; }
		//void setControl( EVarControl control )		{ assert(m_VarControl == EVarControl::Unowned); m_VarControl = control; }
		void setOwner( const IEndpoint* etp )		{ m_Owner = etp->getCopy(); }

		u32 id() const								{ return m_Id; }
		const string& typeName() const				{ return m_TypeName; }
		EVarControl getVarControl() const			{ return m_VarControl; }
		const IEndpoint* getOwner() const			{ return m_Owner.get(); }
		const BinSerializer& initData() const		{ return m_InitData; }

		// Flag dirty when any of the variables inside the group get written/changed.
		void markChanged();
		void unGroup();

		class Network& network() const;
		
		
	private:
		GroupCollection& m_GroupCollection;
		u32 m_Id;
		string m_TypeName;
		bool m_WasUngrouped;
		bool m_Changed;
		bool m_RemotelyCreated;
		EVarControl m_VarControl;
		sptr<IEndpoint> m_Owner;
		mutex m_VariablesMutex;
		vector<NetVariable*> m_Variables;
		BinSerializer m_InitData;
	};
}

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
		Group(GroupCollection& groupCollection, vector<NetVariable*>& vars, u16 type);
		~Group() override;

		// Post set after Id, control and owner are know.
		void setId(u32 _id)							{ assert(m_Id==-1); m_Id = _id; }
		void setControl( EVarControl control )		{ assert(m_VarControl == EVarControl::Unowned); m_VarControl = control; }
		void setOwner( const IEndpoint* etp )		{ assert(m_Owner==nullptr); m_Owner = etp->getCopy(); }

		u32 id() const								{ return m_Id; }
		u16 typeId() const							{ return m_Type; }
		EVarControl getVarControl() const			{ return m_VarControl; }
		const IEndpoint* getOwner() const			{ return m_Owner.get(); }

		// Flag dirty when any of the variables inside the group get written/changed.
		void markChanged();
		void unGroup();
		
		
	private:
		GroupCollection& m_GroupCollection;
		u32 m_Id;
		u16 m_Type;
		bool m_WasUngrouped;
		bool m_Changed;
		bool m_RemotelyCreated;
		EVarControl m_VarControl;
		sptr<IEndpoint> m_Owner;
		mutex m_VariablesMutex;
		vector<NetVariable*> m_Variables;
	};
}

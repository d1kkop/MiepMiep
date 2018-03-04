#include "Group.h"
#include "Variables.h"


namespace MiepMiep
{
	Group::Group(GroupCollection& groupCollection, vector<NetVariable*>& vars, u16 type):
		m_GroupCollection(groupCollection),
		m_Variables(vars),
		m_Id(-1),
		m_Type(type),
		m_WasUngrouped(false),
		m_Changed(false),
		m_VarControl(EVarControl::Unowned)
	{
		for ( auto* v : m_Variables ) 
			v->setGroup( this );
	}

	Group::~Group()
	{
		unGroup();
	}

	void Group::markChanged()
	{
		m_Changed = true;
	}

	void Group::unGroup()
	{
		scoped_lock lk(m_VariablesMutex);
		if (m_WasUngrouped) return;
		m_Variables.clear();
		m_WasUngrouped = true;
		// TODO send remove group msg
	}
}
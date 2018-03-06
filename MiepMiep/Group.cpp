#include "Group.h"
#include "Variables.h"
#include "GroupCollection.h"


namespace MiepMiep
{
	Group::Group(GroupCollection& groupCollection, vector<NetVariable*>& vars, const string& typeName, const BinSerializer& initData):
		m_GroupCollection(groupCollection),
		m_Variables(vars),
		m_InitData(initData),
		m_Id(-1),
		m_TypeName(typeName),
		m_WasUngrouped(false),
		m_Changed(false),
		m_VarControl(EVarControl::Full)
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

	class Network& Group::network() const
	{
		return m_GroupCollection.network();
	}

}
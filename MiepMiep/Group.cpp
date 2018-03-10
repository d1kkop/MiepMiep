#include "Group.h"
#include "Variables.h"
#include "GroupCollection.h"


namespace MiepMiep
{
	Group::Group(GroupCollection& groupCollection, vector<NetVariable*>& vars, const string& typeName, const BinSerializer& initData, EVarControl initControlType):
		m_GroupCollection(groupCollection),
		m_Variables(vars),
		m_InitData(initData),
		m_Id(-1),
		m_TypeName(typeName),
		m_WasUngrouped(false),
		m_Changed(false)
	{
		byte k=0;
		for ( auto* v : m_Variables ) 
			v->initialize( this, initControlType, k++ );
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
		// Only need to clear once
		{
			scoped_lock lk(m_VariablesMutex);
			if (m_WasUngrouped) return;
			m_WasUngrouped = true;
		}
		m_Variables.clear();
	}

	MM_TS void Group::setNewOwnership(byte varIdx, const IEndpoint* newOwner)
	{
		scoped_lock lk(m_VariablesMutex);
		m_Variables[varIdx]->setNewOwner( newOwner );
	}

	MM_TS bool Group::wasUngrouped() const
	{
		scoped_lock lk(m_VariablesMutex);
		return m_WasUngrouped;
	}

	MM_TS void Group::lockVariablesMutex() const
	{
		m_VariablesMutex.lock();
	}

	// NOTE: Requires variables mutex lock
	MM_TS NetVar* Group::getUserVar(byte idx) const
	{
		if ( idx < m_Variables.size() )
			return &m_Variables[ idx ]->getUserVar();
		return nullptr;
	}

	MM_TS void Group::unlockVariablesMutex() const
	{
		m_VariablesMutex.unlock();
	}

	class Network& Group::network() const
	{
		return m_GroupCollection.network();
	}

}
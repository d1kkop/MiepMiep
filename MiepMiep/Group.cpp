#include "Group.h"
#include "Session.h"
#include "Variables.h"
#include "NetVariable.h"
#include "GroupCollection.h"

namespace MiepMiep
{
	Group::Group(GroupCollection& groupCollection, const Session& session, vector<NetVariable*>& vars,
				 const string& typeName, const BinSerializer& initData, EVarControl initControlType):
		m_GroupCollection(groupCollection),
		m_Session(session.to_ptr()),
		m_Variables(vars),
		m_InitData(initData),
		m_Id(-1),
		m_TypeName(typeName),
		m_WasUngrouped(false),
		m_Changed(false)
	{
		byte k=0;
		for ( auto* v : m_Variables )
		{
			v->initialize( this, nullptr, initControlType, k++ );
		}
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
		if (m_WasUngrouped) return;
		m_WasUngrouped = true;
		m_Variables.clear();
	}

	void Group::setNewOwnership(byte varIdx, const IAddress* owner)
	{
		m_Variables[varIdx]->setNewOwner(owner);
	}

	bool Group::wasUngrouped() const
	{
		return m_WasUngrouped;
	}

	NetVar* Group::getUserVar(byte idx) const
	{
		if ( idx < m_Variables.size() )
			return &m_Variables[ idx ]->getUserVar();
		return nullptr;
	}

	Network& Group::network() const
	{
		return m_GroupCollection.network();
	}

	const Session& Group::session() const
	{
		return *m_Session;
	}

}
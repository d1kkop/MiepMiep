#include "Variables.h"
#include "NetVariable.h"
#include "Platform.h"
#include "Group.h"
#include "PerThreadDataProvider.h"
#include <cassert>
#include <algorithm>


namespace MiepMiep
{
	// ---- NetVar (accessible by user) ------------------------------------------------------------

	NetVar::NetVar(void* data, u32 size):
		p(new NetVariable(*this, (byte*)data, size))
	{
	}

	NetVar::~NetVar()
	{
		delete p;
	}

	const IEndpoint* NetVar::getOwner() const
	{
		return p->getOwner();
	}

	EVarControl NetVar::getVarConrol() const
	{
		return p->getVarControl();
	}

	u32 NetVar::getGroupId() const
	{
		return p->getGroupId();
	}

	void NetVar::markChanged()
	{
		p->markChanged();
	}

	void NetVar::addUpdateCallback( const function<void(NetVar&, const byte*, const byte*)>& cb )
	{
		p->addUpdateCallback( cb );
	}


	// ---- NetVariable internal ------------------------------------------------------------

	NetVariable::NetVariable(NetVar& userVar, byte* data, u32 size):
		m_UserVar(userVar),
		m_Group(nullptr),
		m_Data(data),
		m_Size(size),
		m_Changed(false)
	{
		assert( m_Group == nullptr && "VariableGroup::Last" );
		assert( data && size <= MM_MAXMTUSIZE );
		if ( !data )
		{
			Platform::log(ELogType::Warning, MM_FL, "Variable has no valid 'data' ptr. It will not synchronize!");
		}

		PerThreadDataProvider::getConstructedVariables().emplace_back( this );
	}

	NetVariable::~NetVariable()
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		std::remove_if( varVec.begin(), varVec.end(), [&](NetVariable* nvar)
		{
			return nvar == this;
		});

		if ( m_Group )
		{
			m_Group->unGroup();
		}
	}

	const IEndpoint* NetVariable::getOwner() const
	{
		if ( !m_Group ) return nullptr;
		return m_Group->getOwner();
	}

	EVarControl NetVariable::getVarControl() const
	{
		if ( !m_Group )
			return EVarControl::Unowned;
		return m_Group->getVarControl();
	}

	u32 NetVariable::getGroupId() const
	{
		if ( !m_Group )
			return -1;
		return m_Group->id();
	}

	bool NetVariable::sync( BinSerializer& bs, bool write )
	{
		byte prevData[MM_MAXMTUSIZE];

		// capture previous state
		scoped_lock lk(m_UpdateCallbackMutex);
		if ( !write && !m_UpdateCallbacks.empty() )
		{
			Platform::memCpy( prevData, m_Size, m_Data, m_Size );
		}

		__CHECKEDB( m_UserVar.sync( bs, write ) );

		// if reading and data was changed
		if ( !write && !m_UpdateCallbacks.empty() && memcmp( m_Data, prevData, m_Size ) != 0 )
		{
			for ( auto & cb : m_UpdateCallbacks )
			{
				cb( m_UserVar, m_Data, prevData );
			}
		}

		return true;
	}

	void NetVariable::markChanged()
	{
		if ( m_Group )
		{
			m_Group->markChanged();
		}
		m_Changed = true;
	}

	void NetVariable::markUnchanged()
	{
		m_Changed = false;
	}

	MM_TS void NetVariable::addUpdateCallback(const std::function<void(NetVar&, const byte*, const byte*)>& callback)
	{
		scoped_lock lk(m_UpdateCallbackMutex);
		m_UpdateCallbacks.emplace_back(callback);
	}
}
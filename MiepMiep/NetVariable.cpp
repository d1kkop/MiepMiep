#include "NetVariable.h"
#include "Link.h"
#include "Network.h"
#include "Group.h"
#include "NetworkEvents.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
	// ---- Event ------------------------------------------------------------

	struct EventOwnerChanged : IEvent
	{
		EventOwnerChanged(const sptr<Link>& link, const sptr<IAddress> newOwner, const sptr<Group>& group, byte varIdx):
			IEvent( link, false ),
			m_NewOwner( newOwner ),
			m_Group(group),
			m_VarBit(varIdx)
		{
		}

		void process() override
		{
			// User var(s) may be destructed in meantime (is in user memory).
			if ( !m_Group->wasUngrouped() )
			{
				// It is the user's responsibility that the obtained ptr is still valid.
				m_Group->lockVariablesMutex();
				NetVar* userVar = m_Group->getUserVar( m_VarBit );
				if ( !userVar )
				{
					LOG( "Could not process variable owner change event as variable was no longer available." );
					m_Group->unlockVariablesMutex();
					return;
				}

				// Safe to obtain ref to userVar as we have acquired the variables-lock.
				m_Link->getSession()->forListeners( [&] ( ISessionListener* l )
				{
					l->onOwnerChanged( m_Link->session(), *userVar, m_NewOwner.get() );
				});

				m_Group->unlockVariablesMutex();
			}
			else
			{
				LOG( "Change owner event was not posted as user variable was destructed before the event was processed." );
			}
		}

		byte m_VarBit;
		sptr<const Group> m_Group;
		sptr<const IAddress> m_NewOwner;
	};

	// ---- RPC ------------------------------------------------------------

	// ID, bit idx, Endpoint (owner)
	MM_RPC(changeOwner, u32, byte, sptr<IAddress>)
	{
		RPC_BEGIN();

		u32 netId = get<0>(tp);
		sptr<Group> g = nw.getOrAdd<GroupCollectionNetwork>()->findGroup( netId );
		if ( !g )
		{
			LOGW( "Cannot find group with id %d.", netId );
			return;
		}

		byte bit = get<1>(tp);
		const sptr<IAddress>& owner = get<2>(tp);
		if ( !owner ) // we become owner
		{
			g->setNewOwnership( bit, nullptr );
			l.pushEvent<EventOwnerChanged>( nullptr, g,  bit );
		}
		else
		{
			g->setNewOwnership( bit, owner.get() );
			l.pushEvent<EventOwnerChanged>( owner, g,  bit );
		}
	}


	// ---- NetVar (accessible by user) ------------------------------------------------------------

	NetVar::NetVar(void* data, u32 size):
		p(new NetVariable(*this, (byte*)data, size))
	{
	}

	NetVar::~NetVar()
	{
		delete p;
	}

	MM_TS sptr<const IAddress> NetVar::getOwner() const
	{
		return p->getOwner();
	}

	MM_TS EChangeOwnerCallResult NetVar::changeOwner(const IAddress& etp)
	{
		return p->changeOwner( etp );
	}

	MM_TS EVarControl NetVar::getVarConrol() const
	{
		return p->getVarControl();
	}

	MM_TS u32 NetVar::getGroupId() const
	{
		return p->groupId();
	}

	MM_TS void NetVar::markChanged()
	{
		p->markChanged();
	}

	MM_TS void NetVar::addUpdateCallback( const function<void(NetVar&, const byte*, const byte*)>& cb )
	{
		p->addUpdateCallback( cb );
	}


	// ---- NetVariable internal ------------------------------------------------------------

	NetVariable::NetVariable(NetVar& userVar, byte* data, u32 size):
		m_UserVar(userVar),
		m_Group(nullptr),
		m_Data(data),
		m_Size(size),
		m_Bit(-1),
		m_Changed(false),
		m_VarControl(EVarControl::Unowned)
	{
		assert( m_Group == nullptr && "VariableGroup::Last" );
		assert( data && size <= MM_MAX_FRAGMENTSIZE ); // If bigger than fragment size, there is no possiblity to fit this variable in a fragment.
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
		unGroup();
	}

	void NetVariable::initialize(class Group* g, const IAddress* owner, EVarControl initVarControl, byte bit)
	{
		scoped_spinlock lk(m_GroupMutex);
		assert( g && !m_Group && m_Bit==(byte)-1 );
		m_Group = g;
		m_Bit = bit;
		if ( owner )  m_Owner = owner->to_ptr();
		m_VarControl = initVarControl;
	}

	MM_TS void NetVariable::unGroup()
	{
		scoped_spinlock lk(m_GroupMutex);
		if ( m_Group )
		{
			m_Group->unGroup();
			m_Group = nullptr;
		}
	}

	MM_TS u32 NetVariable::groupId() const
	{
		scoped_spinlock lk(m_GroupMutex);
		if ( !m_Group )
			return -1;
		return m_Group->id();
	}

	MM_TS sptr<const IAddress> NetVariable::getOwner() const
	{
		scoped_spinlock lk(m_OwnershipMutex);
		return m_Owner;
	}

	MM_TS enum class EVarControl NetVariable::getVarControl() const
	{
		return m_VarControl;
	}

	MM_TS void NetVariable::markChanged()
	{
		// If any var changes, group changes too.
		{
			scoped_spinlock lk(m_OwnershipMutex);
			if ( m_Group )
			{
				m_Group->markChanged();
			}
		}
		scoped_spinlock lk(m_ChangeMutex);
		m_Changed = true;
	}

	MM_TS bool NetVariable::isChanged() const
	{
		scoped_spinlock lk(m_ChangeMutex);
		return m_Changed;
	}

	MM_TS void NetVariable::markUnchanged()
	{
		scoped_spinlock lk(m_ChangeMutex);
		m_Changed = false;
	}

	MM_TS EChangeOwnerCallResult NetVariable::changeOwner(const IAddress& etp)
	{
		// Cannot change if we are the NOT owner.
		if ( getOwner() ) return EChangeOwnerCallResult::NotOwned;

		Network* nw;
		{
			scoped_spinlock lk(m_GroupMutex);
			if ( !m_Group ) return EChangeOwnerCallResult::NotOwned;
			nw = &m_Group->network();
		}

		auto sendRes = nw->callRpc2<MiepMiep::changeOwner, u32, byte, sptr<IAddress>>
		(
			groupId(), bit(), etp.getCopy(), &m_Group->session(), nullptr,
			false /*locCall*/, false/*buffer*/, true/*relay*/, true/*sysBit*/, 
			MM_VG_CHANNEL, nullptr 
		);
		return ( sendRes == ESendCallResult::Fine ? EChangeOwnerCallResult::Fine : EChangeOwnerCallResult::Fail );
	}

	MM_TS void NetVariable::setNewOwner(const IAddress* etp)
	{
		scoped_spinlock lk(m_OwnershipMutex);
		if ( !etp )
		{
			// We become owner.
			m_Owner.reset();
			m_VarControl = EVarControl::Full;
			return;
		}
		m_Owner = etp->to_ptr();
		m_VarControl = EVarControl::Remote;
	}

	MM_TS bool NetVariable::readOrWrite(BinSerializer& bs, bool write)
	{
		byte prevData[MM_MAX_FRAGMENTSIZE];

		// capture previous state
		scoped_lock lk(m_UpdateCallbackMutex);
		if ( !write && !m_UpdateCallbacks.empty() )
		{
			Platform::memCpy( prevData, m_Size, m_Data, m_Size );
		}

		__CHECKEDB( m_UserVar.readOrWrite( bs, write ) );

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

	MM_TS void NetVariable::addUpdateCallback(const std::function<void(NetVar&, const byte*, const byte*)>& callback)
	{
		scoped_lock lk(m_UpdateCallbackMutex);
		m_UpdateCallbacks.emplace_back(callback);
	}

}
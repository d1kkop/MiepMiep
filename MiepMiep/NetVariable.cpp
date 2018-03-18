#include "Variables.h"
#include "Link.h"
#include "Network.h"
#include "NetVariable.h"
#include "Platform.h"
#include "Group.h"
#include "NetworkListeners.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "Endpoint.h"
#include "Socket.h"
#include <cassert>
#include <algorithm>


namespace MiepMiep
{
	// ---- Event ------------------------------------------------------------

	struct EventOwnerChanged : EventBase
	{
		EventOwnerChanged(const Link& link, const IAddress* newOwner, const Group& group, byte varIdx):
			EventBase(link),
			m_NewOwner( newOwner ? newOwner->to_ptr() : nullptr ),
			m_Group(group.ptr<Group>()),
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
				m_NetworkListener->processEvents<IConnectionListener>( [&] (IConnectionListener* l) 
				{
					l->onOwnerChanged( *m_Link, *userVar, m_NewOwner.get() );
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

	// ID, bit idx, Endpoint (as string)
	MM_RPC(changeOwner, u32, byte, string)
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
		const string& ipAndPort = get<2>(tp);
		if ( ipAndPort.empty() ) // we become owner
		{
			g->setNewOwnership( bit, nullptr, &l.socket() );
			l.pushEvent<EventOwnerChanged, const IAddress*, Group&, byte&>( nullptr, *g,  bit );
		}
		else
		{
			i32 err;
			sptr<IAddress> destination = IAddress::fromIpAndPort( ipAndPort, &err );
			if ( destination && err == 0 )
			{
				g->setNewOwnership( bit, destination.get(), nullptr );
				l.pushEvent<EventOwnerChanged, const IAddress*, Group&, byte&>( destination.get(), *g,  bit );
			}
			else
			{
				LOGC( "Cannot set new owner, endpoint could not be resolved, error %d.", err );
			}
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

	void NetVariable::initialize(class Group* g, const ISender* sender, const IAddress* owner, EVarControl initVarControl, byte bit)
	{
		scoped_spinlock lk(m_GroupMutex);
		assert( g && !m_Group && m_Bit==(byte)-1 );
		m_Group = g;
		m_Bit = bit;
		if ( sender ) m_Sender = sender->to_ptr();
		if ( owner )  m_Owner  = owner->to_ptr();
		assert( m_Sender || m_Owner );
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

	MM_TS sptr<const ISender> NetVariable::getSender() const
	{
		scoped_spinlock lk(m_OwnershipMutex);
		assert ( m_Sender || m_Owner );
		return m_Sender;
	}

	MM_TS sptr<const IAddress> NetVariable::getOwner() const
	{
		scoped_spinlock lk(m_OwnershipMutex);
		assert ( m_Sender || m_Owner );
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

		// TODO change to endpoint serialization
		string ipAndPort = etp.toIpAndPort();

		Network* nw;
		{
			scoped_spinlock lk(m_GroupMutex);
			if ( !m_Group ) return EChangeOwnerCallResult::NotOwned;
			nw = &m_Group->network();
		}

		auto sender = getSender();
		assert( sender );
		auto sendRes = nw->callRpc2<MiepMiep::changeOwner, u32, byte, string>( 
			groupId(), bit(), ipAndPort, *sender, false, nullptr, false, 
			false, true, true, MM_VG_CHANNEL, nullptr 
		);
		return ( sendRes == ESendCallResult::Fine ? EChangeOwnerCallResult::Fine : EChangeOwnerCallResult::Fail );
	}

	MM_TS void NetVariable::setNewOwner(const IAddress* etp, const ISender* sender)
	{
		scoped_spinlock lk(m_OwnershipMutex);
		assert ( etp || sender );
		if ( !etp )
		{
			// We become owner.
			m_Owner.reset();
			m_Sender = sc<const ISocket*>(sender)->to_ptr();
			m_VarControl = EVarControl::Full;
			return;
		}
		m_Sender.reset();
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
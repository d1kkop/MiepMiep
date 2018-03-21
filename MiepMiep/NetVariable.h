#pragma once

#include "Common.h"
#include "Threading.h"
#include <cassert>
#include <mutex>
#include <vector>
#include <functional>
using namespace std;


namespace MiepMiep
{
	class Group;
	class NetVar;


	class NetVariable
	{
	public:
		NetVariable(NetVar& userVar, byte* data, u32 size);
		~NetVariable();
		MM_TS void initialize(Group* g, const ISender* sender, const IAddress* owner, EVarControl initVarControl, byte bit);

		// Set on initialization (through constructor, no locks required).
		byte* data()			{ return m_Data; }	
		u32 size() const		{ return m_Size; }
		byte bit() const		{ return m_Bit; }

		MM_TS void unGroup();

		MM_TS u32 groupId() const;
		MM_TS sptr<const ISender> getSender() const;
		MM_TS sptr<const IAddress> getOwner() const;
		MM_TS enum class EVarControl getVarControl() const;

		// Requires VariablesMutex to be thread safe and to call user callback code!
		NetVar& getUserVar() const { return m_UserVar; }
		
		// State changers.
		MM_TS void markChanged();
		MM_TS bool isChanged() const;
		MM_TS void markUnchanged();

		MM_TS EChangeOwnerCallResult changeOwner( const IAddress& etp );
		MM_TS void setNewOwner( const IAddress* etp, const ISender* sender );

		MM_TS bool readOrWrite( class BinSerializer& bs, bool write );
		MM_TS void addUpdateCallback( const std::function<void (NetVar&, const byte*, const byte*)>& callback );


	private:
		// All these initialized upon creation.
		NetVar& m_UserVar;
		mutable SpinLock m_GroupMutex;
		Group* m_Group;
		byte* m_Data;
		const u32 m_Size;
		byte m_Bit;

		// Allow marking changed from different threads.
		mutable  SpinLock m_ChangeMutex;
		bool  m_Changed;

		// Ownership can be changed on a per variable basis inside the group.
		mutable SpinLock m_OwnershipMutex;
		atomic<EVarControl> m_VarControl;
		sptr<const IAddress> m_Owner;
		sptr<const ISender> m_Sender;

		// Allow thread safe adding of multiple callbacks for a single variable.
		mutable mutex m_UpdateCallbackMutex;
		vector<function<void (NetVar&, const byte*, const byte*)>> m_UpdateCallbacks;
	};
}
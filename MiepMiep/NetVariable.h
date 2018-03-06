#pragma once

#include "MiepMiep.h"
#include <cassert>
#include <mutex>
#include <vector>
#include <functional>
using namespace std;


namespace MiepMiep
{
	class NetVariable
	{
	public:
		NetVariable(class NetVar& userVar, byte* data, u32 size);
		~NetVariable();

		void setGroup(class Group* g)		{ assert(!m_Group); m_Group = g; }
		void unGroup()						{ m_Group = nullptr; }
		
		byte* data()		{ return m_Data; }	
		u32 size() const	{ return m_Size; }

		u32 id() const;
		const IEndpoint* getOwner() const;
		enum class EVarControl getVarControl() const;
		

		EChangeOwnerCallResult changeOwner( const IEndpoint& etp );

		// Mark changed when variable changes data.
		void markChanged();
		bool isChanged() const { return m_Changed; }
		void markUnchanged();				// Mark unchanged when variable is written to network stream.
		
		bool sync( class BinSerializer& bs, bool write );

		MM_TS void addUpdateCallback( const std::function<void (NetVar&, const byte*, const byte*)>& callback );


	private:
		// All these vars upon creation.
		class NetVar& m_UserVar;
		class Group* m_Group;
		byte* m_Data;
		const u32 m_Size;

		// Allow marking changed from different threads.
		bool  m_Changed;

		// Allow thread safe adding of multiple callbacks for a single variable.
		mutex m_UpdateCallbackMutex;
		vector<function<void (NetVar&, const byte*, const byte*)>> m_UpdateCallbacks;
	};
}
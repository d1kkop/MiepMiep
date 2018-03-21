#pragma once

#include "Types.h"
#include <functional>


namespace MiepMiep
{
	enum class EVarControl
	{
		Unowned,	// Not in network (yet).
		Full,		// The variable is controlled at this Znode. A change to it will be broadcasted to others.
		Semi,		// The variable is partially controlled on this Znode. A change will be broadcasted, but it may be overwritten later by an authoritive node if it did not agree on the change.
		Remote		// The variable is controlled remotely. Any change to it on this Znode is 'unsafe' and will be overwritten.
	};


	enum class EChangeOwnerCallResult
	{
		Fine,		// Change was broadcasted.
		NotOwned,	// Can only change ownership if owning the variable.
		Fail		// See log why. TODO specify why
	};


	/*	A piece of memory that can be used ty synchronize any type of data. 
		There is no practical limitation on the amount of variables.
		Internally, variables are grouped in clusters of max 16. */
	class MM_DECLSPEC NetVar
	{
	public:
		NetVar(void* data, u32 size);
		virtual ~NetVar();


		/*	Returns the remote owner of this endpoint or nullptr if owned here. */
		MM_TS sptr<const IAddress> getOwner() const;


		/*	Change ownership of this variable. Is only allowed if we own the variable. */
		MM_TS EChangeOwnerCallResult changeOwner( const IAddress& etp );


		/*	Specifies how this variable is controlled.
			[Full]		This means that the variable is controlled locally, therefore any writes to this variable from this machine
						will be synchronized to others. The variable will never be overwritten from the network. 
			[Semi]		This means that the variable is controlled locally, that is, it can be written to but it functions as a prediction,
						the variable may be overwritten later by a remote connection if the remote (say: 'server') did not accept the change
						or wants the value to be different.
			[Remote]	The variable is completely owned remotely. That is, if you write to the variable, it will automatically be 
						overwritten by updates from the network. Furthermore, local changes will not be committed to the network. */
		MM_TS EVarControl getVarConrol() const;


		/*	The NetVar is part of a group that has an id. This id is network wide unique.
			Use this networkGroupId to target specific group's remotely. */
		MM_TS u32 getGroupId() const;


		/*	If a variable is written to, this can be called in addition to let the network stream know it has changed
			and should be retransmitted. 
			NOTE: This function does NOT need to be called when assigning data using the assigment (=) operator. */
		MM_TS void markChanged();


		/*	Add callbacks which are called when the variable is updated from the network. */
		MM_TS void addUpdateCallback( const std::function<void (NetVar& nvar, const byte* newData, const byte* prevData)>& cb );


		/*	Callback called from network system to either write (send) or read (update) the variable. */
		virtual bool readOrWrite( BinSerializer& bs, bool write ) = 0;


	private:
		class NetVariable* p;
	};


	/*	The GenericNetVar is a template. */
	template <typename T>
	class GenericNetVar: public NetVar
	{
	public:
		GenericNetVar(): NetVar( &m_Data, sizeof(T) ) { }
		GenericNetVar(const T& o) : GenericNetVar() { m_Data = o; }

		GenericNetVar<T>& operator = (const T& o)
		{
			m_Data = o;
			markChanged();
			return *this;
		}

		operator T& () { return m_Data; }

		bool readOrWrite( BinSerializer& bs, bool write ) override { return MiepMiep::readOrWrite(bs, m_Data, write); }

	protected:
		T m_Data;
	};


	using NetChar  = GenericNetVar<char>;
	using NetInt16 = GenericNetVar<i16>;
	using NetInt32 = GenericNetVar<i32>;

	using NetByte   = GenericNetVar<byte>;
	using NetUint16 = GenericNetVar<u16>;
	using NetUint32 = GenericNetVar<u32>;
}
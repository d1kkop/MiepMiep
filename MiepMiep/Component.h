#pragma once

#include "Common.h"
#include <map>
#include <vector>
#include <deque>


namespace MiepMiep
{
	enum class EComponentType
	{
		LinkState,
		LinkMasterState,
		LinkStats,
		MasterLinkData,
		RemoteServerInfo,
		Listener,
		LinkManager,
		MasterServer,
		SocketSetManager,
		ListenerManager,
		SendThreadManager,
		JobSystem,
		NetworkEvents,
		NetworkActions,
		GroupCollectionLink,
		GroupCollectionNetwork,
		GroupCreateFunctions,
		// stream components
		ReliableSend,
		ReliableRecv,
		UnreliableSend,
		UnreliableRecv,
		ReliableNewSend,
		ReliableNewRecv,
		// ack stream components
		ReliableAckSend,
		ReliableAckRecv,
		ReliableNewAckSend,
		ReliableNewAckRecv
	};


	class IComponent
	{
	public:
		virtual ~IComponent() = default;
		u32 idx() const { return m_Idx; }

	protected:
		u32 m_Idx;

		friend class ComponentCollection;
	};


	class ComponentCollection
	{
	public:
		template <typename T> bool has(u32 idx=0) const;
		template <typename T> sptr<T> get(u32 idx=0) const;
		template <typename T> u32 count() const;
		template <typename T> bool remove(u32 idx=0);
		template <typename T, typename CB> void forAll(const CB& cb);
		template <typename T, typename CB> void forAll(const CB& cb) const;

	protected:
		template <typename T, typename ...Args> sptr<T> getOrAddInternal(u32 idx=0, Args&&... args);

	protected:
		map<EComponentType, vector<sptr<IComponent>>> m_Components;
	};


	template <typename T>
	bool ComponentCollection::has(u32 idx) const
	{
		auto it = m_Components.find( T::compType() );
		return (it != m_Components.end()) && (it->second.size() > idx) && (it->second[idx].get() != nullptr);
	}

	template <typename T>
	sptr<T> ComponentCollection::get(u32 idx) const
	{
		auto compIt = m_Components.find( T::compType() );
		if ( compIt != m_Components.end() )
		{
			auto& vecComp = compIt->second;
			if ( vecComp.size() > idx )
			{
				return static_pointer_cast<T> ( vecComp[idx] );
			}
		}
		return nullptr;
	}

	template <typename T>
	u32 ComponentCollection::count() const
	{
		auto compIt = m_Components.find( T::compType() );
		if ( compIt != m_Components.end() )
			return (u32)compIt->second.size();
		else
			return 0;
	}

	template <typename T>
	bool ComponentCollection::remove(u32 idx)
	{
		if ( has<T>(idx) )
		{
			m_Components[ T::compType() ][idx].reset();
			return true;
		}
		return false;
	}

	template <typename T, typename CB> 
	void ComponentCollection::forAll(const CB& cb)
	{
		auto cIt = m_Components.find( T::compType() );
		if ( cIt != m_Components.end() )
		{
			auto& v = cIt->second;
			for ( auto& c : v )
			{
				if ( !cb( sc<T&>(*c) ) )
					break; // stop iterating if lamda returned false
			}
		}
	}

	template <typename T, typename CB>
	void ComponentCollection::forAll(const CB& cb) const
	{
		auto cIt = m_Components.find(T::compType());
		if (cIt != m_Components.end())
		{
			auto& v = cIt->second;
			for (auto& c : v)
			{
				if (!cb(sc<const T&>(*c)))
					break; // stop iterating if lamda returned false
			}
		}
	}

	template <typename T, typename ...Args>
	sptr<T> ComponentCollection::getOrAddInternal(u32 idx, Args&&... args)
	{
		if (!has<T>(idx))
		{
			sptr<T> comp  = reserve_sp<T, Args...>(MM_FL, args... );
			auto& vecComp = m_Components[T::compType()];
			if ( vecComp.size() <= idx )
				vecComp.resize( idx + 1 );
			comp->m_Idx = idx;
			assert( vecComp[idx] == nullptr );
			vecComp[idx] = static_pointer_cast<IComponent>( comp );
		}
		return static_pointer_cast<T>( m_Components[T::compType()][idx] );
	}
}

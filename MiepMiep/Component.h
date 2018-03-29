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
	};


	class ComponentCollection
	{
	public:
		template <typename T> MM_TS bool has(u32 idx=0) const;
		template <typename T> MM_TS sptr<T> get(u32 idx=0) const;
		template <typename T> MM_TS u32 count() const;
		template <typename T> MM_TS bool remove(u32 idx=0);
		template <typename T, typename CB> MM_TS void forAll(const CB& cb);
		template <typename T, typename CB> MM_TS void forAll(const CB& cb) const;

	protected:
		template <typename T, typename ...Args> MM_TS sptr<T> getOrAddInternal(u32 idx=0, Args&&... args);

	private:
		mutable rmutex m_ComponentsMutex;
		map<EComponentType, vector<sptr<IComponent>>> m_Components;
	};


	template <typename T>
	MM_TS bool ComponentCollection::has(u32 idx) const
	{
		rscoped_lock lk(m_ComponentsMutex);
		auto it = m_Components.find( T::compType() );
		return (it != m_Components.end()) && (it->second.size() > idx) && (it->second[idx].get() != nullptr);
	}

	template <typename T>
	MM_TS sptr<T> ComponentCollection::get(u32 idx) const
	{
		rscoped_lock lk(m_ComponentsMutex);
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
	MM_TS u32 ComponentCollection::count() const
	{
		rscoped_lock lk(m_ComponentsMutex);
		auto compIt = m_Components.find( T::compType() );
		if ( compIt != m_Components.end() )
			return (u32)compIt->second.size();
		else
			return 0;
	}

	template <typename T>
	MM_TS bool ComponentCollection::remove(u32 idx)
	{
		rscoped_lock lk(m_ComponentsMutex);
		if ( has<T>(idx) )
		{
			m_Components[ T::compType() ][idx].reset();
			return true;
		}
		return false;
	}

	template <typename T, typename CB> 
	MM_TS void ComponentCollection::forAll(const CB& cb)
	{
		rscoped_lock lk(m_ComponentsMutex);
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
	MM_TS void ComponentCollection::forAll(const CB& cb) const
	{
		rscoped_lock lk(m_ComponentsMutex);
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
	MM_TS sptr<T> ComponentCollection::getOrAddInternal(u32 idx, Args&&... args)
	{
		rscoped_lock lk(m_ComponentsMutex);
		if (!has<T>(idx))
		{
			sptr<T> comp = reserve_sp<T, Args...>(MM_FL, args... );
			auto& vecComp = m_Components[T::compType()];
			if ( vecComp.size() <= idx )
				vecComp.resize( idx + 1 );
			vecComp[idx] = static_pointer_cast<IComponent>( comp );
		}
		return static_pointer_cast<T>( m_Components[T::compType()][idx] );
	}
}

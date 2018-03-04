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
		RemoteServerInfo,
		ListenInfo,
		LinkManager,
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
		ReliableAck,
		ReliableNewAck
	};


	class IComponent
	{
	public:
		virtual ~IComponent() = default;
	};


	template <typename T>
	class Parent
	{
	public:
		Parent(T& parent):
			m_Parent(parent)  { }

		T& m_Parent;
	};


	class ComponentCollection
	{
	public:
		template <typename T> MM_TS bool has(byte idx=0) const;
		template <typename T> MM_TS T* get(byte idx=0);

	protected:
		template <typename T, typename ...Args> MM_TS T* getOrAddInternal(byte idx=0, Args... args);

	private:
		mutable rmutex m_ComponentsMutex;
		map<EComponentType, vector<uptr<IComponent>>> m_Components;
	};


	template <typename T>
	MM_TS bool ComponentCollection::has(byte idx) const
	{
		rscoped_lock lk(m_ComponentsMutex);
		auto it = m_Components.find( T::compType() );
		return (it != m_Components.end()) && (it->second.size() > idx);
	}

	template <typename T>
	MM_TS T* ComponentCollection::get(byte idx)
	{
		rscoped_lock lk(m_ComponentsMutex);
		auto compIt = m_Components.find( T::compType() );
		if ( compIt != m_Components.end() )
		{
			auto& vecComp = compIt->second;
			if ( vecComp.size() > idx )
			{
				return static_cast<T*>( vecComp[idx].get() );
			}
		}
		return nullptr;
	}

	template <typename T, typename ...Args>
	MM_TS T* ComponentCollection::getOrAddInternal(byte idx, Args... args)
	{
		rscoped_lock lk(m_ComponentsMutex);
		if (!has<T>(idx))
		{
			T* comp = reserve<T, Args...>(MM_FL, args...);
			auto& vecComp = m_Components[T::compType()];
			if ( vecComp.size() <= idx )
				vecComp.resize( idx + 1 );
			vecComp[idx] = uptr<IComponent>(comp);
		}
		return static_cast<T*>( m_Components[T::compType()][idx].get() );
	}
}

#pragma once

#include "Memory.h"
#include "Component.h"
#include "Endpoint.h"
#include "Network.h"


namespace MiepMiep
{
	class Link: public Parent<Network>, public ComponentCollection, public ITraceable
	{
	public:
		Link(Network& network);

		u32 id() const { return m_Id; }
		const IEndpoint& remoteEtp() const { return *m_RemoteEtp; }

		MM_TS static sptr<Link> create(Network& network, const IEndpoint& other);
		MM_TS class BinSerializer& beginSend();

		MM_TS void createGroup( u16 groupType );
		MM_TS void destroyGroup( u32 id );

		template <typename T, typename ...Args>
		MM_TS T* getOrAdd(byte idx=0, Args... args);


	private:
		u32 m_Id;
		sptr<class IEndpoint> m_RemoteEtp;
		map<EComponentType, vector<uptr<IComponent>>> m_Components;
	};


	template <typename T, typename ...Args>
	MM_TS T* MiepMiep::Link::getOrAdd(byte idx, Args... args)
	{
		return getOrAddInternal<T, Link&>(idx, *this, args...);
	}
}

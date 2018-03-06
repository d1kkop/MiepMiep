#pragma once

#include "Memory.h"
#include "Component.h"
#include "Endpoint.h"
#include "Network.h"


namespace MiepMiep
{
	// ------------ Link -----------------------------------------------

	class Link: public ParentNetwork, public ComponentCollection, public ITraceable
	{
	public:
		Link(Network& network);

		u32 id() const { return m_Id; }
		const IEndpoint& remoteEtp() const { return *m_RemoteEtp; }

		MM_TS static sptr<Link> create(Network& network, const IEndpoint& other);
		MM_TS class BinSerializer& beginSend();

		MM_TS void createGroup( const string& groupType, const BinSerializer& initData );
		MM_TS void destroyGroup( u32 id );

		template <typename T, typename ...Args>
		MM_TS ESendCallResult callRpc(Args... args, bool localCall, bool relay, byte channel, IDeliveryTrace* trace);

		template <typename T, typename ...Args>
		MM_TS T* getOrAdd(byte idx=0, Args... args);

	private:
		u32 m_Id;
		sptr<class IEndpoint> m_RemoteEtp;
		map<EComponentType, vector<uptr<IComponent>>> m_Components;
	};


	template <typename T, typename ...Args>
	MM_TS ESendCallResult MiepMiep::Link::callRpc(Args... args, bool localCall, bool relay, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., *this, bs, localCall);
		return priv_send_rpc( *this, T::rpcName(), bs, &remoteEtp(), false, false, relay, channel, trace );
	}

	template <typename T, typename ...Args>
	MM_TS T* MiepMiep::Link::getOrAdd(byte idx, Args... args)
	{
		return getOrAddInternal<T, Link&>(idx, *this, args...);
	}


	// ------------ ParentLink -----------------------------------------------

	class ParentLink
	{
	public:
		ParentLink(Link& link):
			m_Link(link) { }
			
		Link& m_Link;
	};
}

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
		const sptr<const IEndpoint>& remoteEtp() const { return m_RemoteEtp; }

		MM_TS static sptr<Link> create(Network& network, const IEndpoint& other);
		MM_TS class BinSerializer& beginSend();

		MM_TS void createGroup( const string& groupType, const BinSerializer& initData );
		MM_TS void destroyGroup( u32 id );

		template <typename T, typename ...Args>
		MM_TS ESendCallResult callRpc(Args... args, bool localCall=false, bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr);

		template <typename T, typename ...Args>
		MM_TS void pushEvent(Args... args); 

		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAdd(byte idx=0, Args... args);

	private:
		u32 m_Id;
		sptr<const class IEndpoint> m_RemoteEtp;
		map<EComponentType, vector<uptr<IComponent>>> m_Components;
	};


	template <typename T, typename ...Args>
	MM_TS ESendCallResult MiepMiep::Link::callRpc(Args... args, bool localCall, bool relay, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., m_Network, bs, localCall);
		return priv_send_rpc( m_Network, T::rpcName(), bs, remoteEtp().get(), false, false, relay, channel, trace );
	}

	template <typename T, typename ...Args>
	MM_TS void Link::pushEvent(Args... args)
	{
		sptr<T> evt = make_shared<T>(remoteEtp(), args...);
		sptr<IEvent> evtDown = static_pointer_cast<IEvent>(evt);
		m_Network.getOrAdd<NetworkListeners>()->pushEvent( evtDown );
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getOrAdd(byte idx, Args... args)
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

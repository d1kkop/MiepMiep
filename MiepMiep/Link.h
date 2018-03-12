#pragma once

#include "Memory.h"
#include "Component.h"
#include "Endpoint.h"
#include "Network.h"
#include "Threading.h"
#include "Socket.h"


namespace MiepMiep
{
	// ------------ Link -----------------------------------------------

	class Link: public ParentNetwork, public ComponentCollection, public ITraceable
	{
	public:
		Link(Network& network);
		~Link() override;
		MM_TS void close();
		MM_TS static sptr<Link> create(Network& network, const IEndpoint& remoteEtp, u32* id);

		void setOriginator( const class Listener& listener );
		const class Listener* getOriginator() const;

		u32 id() const { return m_Id; }
		const IEndpoint& remoteEtp() const { return *m_RemoteEtp; }
		const ISocket& socket() const { return *m_Socket; }
		
		MM_TS void createGroup( const string& groupType, const BinSerializer& initData );
		MM_TS void destroyGroup( u32 id );

		template <typename T, typename ...Args>
		MM_TS ESendCallResult callRpc(Args... args, bool localCall=false, bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr);

		template <typename T, typename ...Args>
		MM_TS void pushEvent(Args&&... args); 

		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAdd(u32 idx=0, Args... args);

		template <typename T, typename ...Args>
		MM_TS sptr<T> getInNetwork(u32 idx=0, Args... args);

		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAddInNetwork(u32 idx=0, Args... args);

		MM_TS void receive( BinSerializer& bs );
		MM_TS void send( const byte* data, u32 length );

	private:
		u32 m_Id;
		sptr<const class IEndpoint> m_RemoteEtp;
		sptr<ISocket> m_Socket;
		mutable SpinLock m_OriginatorMutex;
		sptr<const class Listener> m_Originator;
	};


	template <typename T, typename ...Args>
	MM_TS ESendCallResult MiepMiep::Link::callRpc(Args... args, bool localCall, bool relay, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., m_Network, bs, localCall);
		return priv_send_rpc( m_Network, T::rpcName(), bs, &remoteEtp(), false, false, relay, true /* sys bit */, channel, trace );
	}

	template <typename T, typename ...Args>
	MM_TS void Link::pushEvent(Args&&... args)
	{
		sptr<T> evt = make_shared<T>(remoteEtp(), args...);
		sptr<IEvent> evtDown = static_pointer_cast<IEvent>(evt);
		m_Network.getOrAdd<NetworkListeners>()->pushEvent( evtDown );
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getOrAdd(u32 idx, Args... args)
	{
		return getOrAddInternal<T, Link&>(idx, *this, args...);
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getInNetwork(u32 idx, Args... args)
	{
		return m_Network.get<T, Args...>(idx, args...);
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getOrAddInNetwork(u32 idx, Args... args)
	{
		return m_Network.getOrAdd<T, Args...>(idx, args...);
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

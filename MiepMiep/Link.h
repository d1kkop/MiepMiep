#pragma once

#include "Component.h"
#include "PacketHandler.h"


namespace MiepMiep
{
	class IEndpoint;
	class ISocket;
	class Network;
	class Link;

	// ------------ Link -----------------------------------------------

	class Link: public ComponentCollection, public IPacketHandler
	{
	public:
		Link(Network& network);
		~Link() override;
		MM_TS static sptr<Link> create(Network& network, const IEndpoint& remoteEtp, u32* id, const class Listener* originator);

		// These are thread safe because they are set from constructor and never changed afterwards.
		MM_TS u32 id() const { return m_Id; }
		MM_TS const IEndpoint& remoteEtp() const { return *m_RemoteEtp; }
		MM_TS const ISocket& socket() const { return *m_Socket; }
		MM_TS const Listener* originator() const { return m_Originator.get(); }
		MM_TS const char* ipAndPort() const;
		
		MM_TS void createGroup( const string& groupType, const BinSerializer& initData );
		MM_TS void destroyGroup( u32 id );

		MM_TS void handleSpecial( class BinSerializer& bs, const class Endpoint& etp ) override;

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

		MM_TO_PTR( Link )

	private:
		u32 m_Id;
		sptr<const class IEndpoint> m_RemoteEtp;
		sptr<const class ISocket>   m_Socket;
		sptr<const class Listener>  m_Originator;
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
}

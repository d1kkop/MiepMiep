#pragma once

#include "Component.h"
#include "MiepMiep.h"
#include "LinkManager.h"
#include "PacketHandler.h"
#include "Socket.h"
#include "Threading.h"


namespace MiepMiep
{
	class IAddress;
	class ISocket;
	class Network;
	class Session;
	class MasterSession;
	class Link;

	// ------------ Link -----------------------------------------------

	class Link: public ILink, public ComponentCollection, public IPacketHandler
	{
	public:
		Link(Network& network);
		~Link() override;
		MM_TSC bool setSession( SessionBase& session );

		MM_TSC bool operator<  ( const Link& o ) const;
		MM_TSC bool operator== ( const Link& o ) const;
		MM_TSC bool operator!= ( const Link& o ) const { return !(*this == o); }

		MM_TS static sptr<Link> create( Network& network, SessionBase& session, const SocketAddrPair& sap, bool addHandler );
		MM_TS static sptr<Link> create( Network& network, SessionBase& session, const SocketAddrPair& sap, u32 id, bool addHandler );
		MM_TS static sptr<Link> create( Network& network, SessionBase* session, const SocketAddrPair& sap, u32 id, bool addHandler );

		// ILink
		MM_TSC INetwork& network() const override;
		MM_TSC ISession& session() const override;
		MM_TSC const IAddress& destination() const override { return *m_SockAddrPair.m_Address; }
		MM_TSC const IAddress& source() const override { return *m_Source; }
		MM_TS  bool  isConnected() const override;
		MM_TSC SessionBase* getSession() const override;

		// These are thread safe beacuse they are set before the newly created link object is returned from creation.
		MM_TSC u32 id() const { return m_Id; }
		MM_TSC const ISocket& socket() const { return *m_SockAddrPair.m_Socket; }
		MM_TSC const char* ipAndPort() const;
		MM_TSC const char* info() const;
		MM_TSC const SocketAddrPair& getSocketAddrPair() const;

		MM_TSC Session& normalSession() const;
		MM_TSC MasterSession& masterSession() const;
		
		MM_TS void updateCustomMatchmakingMd( const MetaData& md );
		MM_TS bool disconnect(bool isKick, bool sendMsg);
		
		MM_TS void createGroup( const string& groupType, const BinSerializer& initData );
		MM_TS void destroyGroup( u32 id );

		template <typename T, typename ...Args>
		MM_TS ESendCallResult callRpc(Args... args, bool localCall=false, bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr);

		template <typename T, typename ...Args>
		MM_TS void pushEvent(Args&&... args); 

		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAdd(u32 idx=0, Args&&... args);

		template <typename T, typename ...Args>
		MM_TS sptr<T> getInNetwork(u32 idx=0, Args&&... args);

		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAddInNetwork(u32 idx=0, Args&&... args);

		MM_TS void receive( BinSerializer& bs );
		MM_TS void send( const byte* data, u32 length );

		MM_TO_PTR( Link )

	private:
		// All these fields could have their own component, but that is a lot of boilerplate for a single field.
		u32 m_Id;
		bool m_SocketWasAddedToHandler;
		sptr<SessionBase> m_Session;
		SocketAddrPair m_SockAddrPair;
		sptr<const class IAddress> m_Source;
		SpinLock m_MatchmakingDataMutex;
		MetaData m_CustomMatchmakingMd;
	};


	template <typename T, typename ...Args>
	MM_TS ESendCallResult MiepMiep::Link::callRpc(Args... args, bool localCall, bool relay, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., m_Network, bs, localCall);
		return priv_send_rpc( m_Network, T::rpcName(), bs, nullptr, this, false /* buffer */, relay, true /* sys bit */, channel, trace );
	}

	template <typename T, typename ...Args>
	MM_TS void Link::pushEvent(Args&&... args)
	{
		sptr<T> evt = make_shared<T>(to_ptr(), args...);
		sptr<IEvent> evtDown = static_pointer_cast<IEvent>(evt);
		m_Network.get<NetworkEvents>()->pushEvent( evtDown );
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getOrAdd(u32 idx, Args&&... args)
	{
		return getOrAddInternal<T, Link&>(idx, *this, args...);
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getInNetwork(u32 idx, Args&&... args)
	{
		return m_Network.get<T, Args...>(idx, args...);
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> Link::getOrAddInNetwork(u32 idx, Args&&... args)
	{
		return m_Network.getOrAdd<T, Args...>(idx, args...);
	}

	template <typename T>
	inline Link& toLink(T& link)
	{
		return sc<Link&>(link);
	}

	//template <>
	inline Link& toLink(const ILink& link)
	{
		return sc<Link&>( const_cast<ILink&>(link) );
	}

}

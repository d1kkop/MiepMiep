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
		void setSession( const sptr<Session>& session ); // Set immediately on construction before link is returend, therefore no locks required.

		bool operator<  ( const Link& o ) const;
		bool operator== ( const Link& o ) const;

		MM_TS void setMasterSession( const sptr<MasterSession>& session );

		MM_TS static sptr<Link> create(Network& network, const Session* session, const IAddress& destination, bool addHandler);
		MM_TS static sptr<Link> create(Network& network, const Session* session, const IAddress& destination, u32 id, bool addHandler);
		MM_TS static sptr<Link> create(Network& network, const Session* session, const SocketAddrPair& sap, u32 id, bool addHandler);

		// ILink
		MM_TS INetwork& network() const override;
		MM_TS ISession& session() const override;
		MM_TS const IAddress& destination() const override { return *m_Destination; }
		MM_TS const IAddress& source() const override { return *m_Source; }
		MM_TS bool  isConnected() const override;

		// These are thread safe beacuse they are set before the newly created link object is returned from creation.
		MM_TS u32 id() const { return m_Id; }
		MM_TS const ISocket& socket() const { return *m_Socket; }
		MM_TS const char* ipAndPort() const;
		MM_TS const char* info() const;
		MM_TS MasterSession* masterSession() const;
		MM_TS SocketAddrPair getSocketAddrPair() const;

		MM_TS void updateCustomMatchmakingMd( const MetaData& md );
		
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
		sptr<const class IAddress>  m_Destination;
		sptr<const class IAddress>  m_Source;
		sptr<const class ISocket>   m_Socket;
		sptr<Session> m_Session;
		mutable SpinLock m_MasterSessionMutex;
		sptr<MasterSession> m_MasterSession;
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
		sptr<T> evt = make_shared<T>(*this, args...);
		sptr<IEvent> evtDown = static_pointer_cast<IEvent>(evt);
		m_Network.get<NetworkListeners>()->pushEvent( evtDown );
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

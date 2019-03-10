#pragma once

#include "Component.h"
#include "ParentNetwork.h"
#include "MiepMiep.h"
#include "LinkManager.h"
#include "PacketHelper.h"
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

	class Link: public ParentNetwork, public ILink, public ComponentCollection, public ITraceable
	{
	public:
		Link(Network& network);
		~Link();
		static sptr<Link> create( Network& network, SessionBase* session, const SocketAddrPair& sap );

		bool operator<  ( const Link& o ) const;
		bool operator== ( const Link& o ) const;
		bool operator!= ( const Link& o ) const { return !(*this == o); }

		// ILink
		INetwork& network() const override;
		ISession& session() const override;
		const IAddress& destination() const override { return *m_SockAddrPair.m_Address; }
		const IAddress& source() const override { return *m_Source; }
		bool  isConnected() const override;
		SessionBase* getSession() const;

		const ISocket& socket() const { return *m_SockAddrPair.m_Socket; }
		const char* ipAndPort() const;
		const char* info() const;
		const SocketAddrPair& getSocketAddrPair() const;

		void setSession( SessionBase& session );
		Session& normalSession() const;
		MasterSession& masterSession() const;

		void updateCustomMatchmakingMd( const MetaData& md );
		bool disconnect( bool isKick, bool isReceive, bool removeLink );

		void createGroup( const string& groupType, const BinSerializer& initData );
		void destroyGroup( u32 id );

		template <typename T, typename ...Args>
		ESendCallResult callRpc(Args... args, bool localCall=false, bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr);

		template <typename T, typename ...Args>
		void pushEvent(Args&&... args); 

		template <typename T, typename ...Args>
		sptr<T> getOrAdd(u32 idx=0, Args&&... args);

		template <typename T, typename ...Args>
		sptr<T> getInNetwork(u32 idx=0, Args&&... args);

		template <typename T, typename ...Args>
		sptr<T> getOrAddInNetwork(u32 idx=0, Args&&... args);

		void receive( BinSerializer& bs );
		void send( const byte* data, u32 length );

		MM_TO_PTR( Link )

	private:
		// All these fields could have their own component, but that is a lot of boilerplate for a single field.
		sptr<SessionBase> m_Session;
		SocketAddrPair m_SockAddrPair;
		sptr<const class IAddress> m_Source;
		MetaData m_CustomMatchmakingMd;
	};


	template <typename T, typename ...Args>
	ESendCallResult MiepMiep::Link::callRpc(Args... args, bool localCall, bool relay, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., m_Network, bs, localCall, channel);
		return priv_send_rpc( m_Network, T::rpcName(), bs, nullptr, this, false /* buffer */, relay, true /* sys bit */, channel, trace );
	}

	template <typename T, typename ...Args>
	void Link::pushEvent(Args&&... args)
	{
		sptr<T> evt = make_shared<T>(to_ptr(), args...);
		sptr<IEvent> evtDown = static_pointer_cast<IEvent>(evt);
		auto ne = m_Network.get<NetworkEvents>();
		if ( ne )
		{
			ne->pushEvent( evtDown );
		}
		else
		{
			assert(false);
			LOGW( "Received event while NetworkEvents was system was destroyed." );
		}
	}

	template <typename T, typename ...Args>
	sptr<T> Link::getOrAdd(u32 idx, Args&&... args)
	{
		return getOrAddInternal<T, Link&>(idx, *this, args...);
	}

	template <typename T, typename ...Args>
	sptr<T> Link::getInNetwork(u32 idx, Args&&... args)
	{
		return m_Network.get<T, Args...>(idx, args...);
	}

	template <typename T, typename ...Args>
	sptr<T> Link::getOrAddInNetwork(u32 idx, Args&&... args)
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

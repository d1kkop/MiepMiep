#pragma once

#include "Memory.h"
#include "Component.h"
#include "MiepMiep.h"
using namespace std;


namespace MiepMiep
{
	class  Link;
	class  Endpoint;
	class  ISocket;
	struct SocketAddrPair;
	class  ServerList;

	enum class ENetworkError
	{
		Fine,
		SerializationError,
		LogicalError
	};


	// -------- Put here because string requires dll-interface and because 
	// -------- we do not want to bother the user with 'helper structs'.		----------------------------------------------------

	struct MasterSessionData : public IComponent, public ITraceable
	{
		MasterSessionData() { memset( this, 0, sizeof( *this ) ); }

		bool  m_IsP2p;
		bool  m_IsPrivate;
		float m_Rating;
		u32	  m_MaxClients;
		std::string m_Name;
		std::string m_Type;
		std::string m_Password;

		// Set from ServerList
		u64	  m_Id;
		sptr<ServerList> m_ServerList;
	};

	struct MM_DECLSPEC SearchFilter
	{
		SearchFilter() { memset( this, 0, sizeof( *this ) ); }

		std::string m_Name;
		std::string m_Type;
		float m_MinRating, m_MaxRating;
		u32 m_MinPlayers, m_MaxPlayers;
		bool m_FindPrivate;
		bool m_FindP2p;
		bool m_FindClientServer;
		MetaData m_CustomMatchmakingMd;
	};


	// ------------ Network -----------------------------------------------


	class Network: public ComponentCollection, public INetwork, public ITraceable
	{
	public:
		Network(bool allowAsyncCallbacks);
		~Network() override;

		void processEvents() override;

		// INetwork
		MM_TS EListenCallResult startListen( u16 port ) override;
		MM_TS bool stopListen( u16 port ) override;

		MM_TS void registerServer( const std::function<void( const ILink& link, bool )>& callback,
								   const IAddress& masterAddr, bool isP2p, bool isPrivate, float rating,
								   u32 maxClients, std::string name, std::string type, std::string password,
								   const MetaData& hostMd, const MetaData& customMatchmakingMd ) override;

		MM_TS void joinServer( const std::function<void( const ILink& link, EJoinServerResult )>& callback,
							   const IAddress& masterAddr, std::string name, std::string type,
							   float minRating, float maxRating, u32 minPlayers, float maxPlayers,
							   bool findPrivate, bool findP2p, bool findClientServer,
							   const MetaData& joinMd, const MetaData customMatchMakingMd ) override;

		MM_TS void createGroupInternal( const ISender& sender, const string& typeName, const BinSerializer& initData, byte channel, IDeliveryTrace* trace );
		MM_TS void destroyGroup( u32 groupId ) override;
		MM_TS void createRemoteGroup( const string& typeName, u32 netId, const BinSerializer& initData, const IAddress& etp );

		MM_TS ESendCallResult sendReliable(const ISender& sender, byte id, const BinSerializer* bs, u32 numSerializers, const IAddress* specific,
										   bool exclude, bool buffer, bool relay,
										   byte channel, IDeliveryTrace* trace) override;
		MM_TS ESendCallResult sendReliable(const ISender& sender, byte id, const BinSerializer** bs, u32 numSerializers, const IAddress* specific, 
										   bool exclude, bool buffer, bool relay, bool systemBit,
										   byte channel, IDeliveryTrace* trace);
		MM_TS ESendCallResult sendReliable(const ISender& sender, const vector<sptr<const struct NormalSendPacket>>&, const IAddress* specific, 
										   bool exclude, bool buffer, byte channel, IDeliveryTrace* trace);

		MM_TS void addConnectionListener( IConnectionListener* listener ) override;
		MM_TS void removeConnectionListener( const IConnectionListener* listener ) override;

		MM_TS bool isListenerSocket( const ISocket& sock ) const;
		MM_TS sptr <const ISender> getFirstNetworkIdentity(bool& hasMultiple) const;


		MM_TS static void clearAllStatics();
		MM_TS MM_DECLSPEC static void printMemoryLeaks();

		// Gets, checks or adds directly in a link in the network -> linkManagerComponent.
		MM_TS sptr<Link> getLink(const SocketAddrPair& sap) const;
		template <typename T>
		MM_TS bool hasOnLink(const Endpoint& etp, u32 idx=0) const;
		template <typename T>
		MM_TS sptr<T> getOnLink(const Endpoint& etp, u32 idx=0) const;
		template <typename T>
		MM_TS sptr<T> getOrAddOnLink(const Endpoint& etp, u32 idx=0);

		// Gets or adds component to ComponentCollection.
		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAdd(u32 idx=0, Args... args);

		// Send rpc with system bit (true or false). Other than that, exactly same as Inetwork.callRpc
		template <typename T, typename ...Args> 
		MM_TS ESendCallResult callRpc2( Args... args, const ISender& sender, bool localCall=false, const IAddress* specific=nullptr, bool exclude=false, bool buffer=false, 
										bool relay=false, bool systemBit=true, byte channel=0, IDeliveryTrace* trace=nullptr );

		bool allowAsyncCallbacks() const { return m_AllowAsyncCallbacks; }


	private:
		bool m_AllowAsyncCallbacks;
		mutex m_ListenerAddMutex;
	};


	template <typename T>
	MM_TS bool MiepMiep::Network::hasOnLink(const Endpoint& etp, u32 idx) const
	{
		const sptr<Link>& link = getLink(etp);
		if ( !link ) return false;
		return link->has<T>(idx);
	}

	template <typename T>
	MM_TS sptr<T> MiepMiep::Network::getOnLink(const Endpoint& etp, u32 idx) const
	{
		Link* link = getLink(etp);
		if ( !link ) return nullptr;
		return link->get<T>(idx);
	}

	template <typename T>
	MM_TS sptr<T> MiepMiep::Network::getOrAddOnLink(const Endpoint& etp, u32 idx)
	{
		Link* link = getLink(etp);
		if ( !link ) return nullptr;
		return link->getOrAdd<>T();
	}

	template <typename T, typename ...Args>
	MM_TS sptr<T> MiepMiep::Network::getOrAdd(u32 idx, Args... args)
	{
		return getOrAddInternal<T, Network&>(idx, *this, args...);
	}

	template <typename T, typename ...Args>
	MM_TS ESendCallResult MiepMiep::Network::callRpc2(Args... args, const ISender& sender, bool localCall, const IAddress* specific, 
													  bool exclude, bool buffer, bool relay, bool systemBit,
													  byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., *this, bs, localCall);
		return priv_send_rpc( *this, T::rpcName(), bs, specific, exclude, buffer, relay, systemBit, channel, trace, &sender );
	}


	template <typename T>
	Network& toNetwork(T& t) 
	{ 
		return sc<Network&>(t);
	}

}
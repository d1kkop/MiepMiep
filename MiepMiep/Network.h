#pragma once

#include "Memory.h"
#include "Component.h"
#include "MiepMiep.h"
#include "Session.h"
using namespace std;


namespace MiepMiep
{
	class  Link;
	class  Session;
	class  Endpoint;
	class  ISocket;
	struct SocketAddrPair;
	class  MasterSessionList;

	enum class ENetworkError
	{
		Fine,
		SerializationError,
		LogicalError
	};

	// ------------ Network -----------------------------------------------


	class Network: public ComponentCollection, public INetwork, public ITraceable
	{
	public:
		Network(bool allowAsyncCallbacks, u32 numWorkerThreads=4);
		~Network() override;

		void processEvents() override;

		// INetwork
		MM_TS EListenCallResult startListen( u16 port ) override;
		MM_TS void stopListen( u16 port ) override;

		MM_TS sptr<ISession> registerServer( const std::function<void( ISession&, bool )>& callback,
											 const IAddress& masterAddr, const std::string& name, const std::string& type,
											 bool isP2p, bool canJoinAfterStart, float rating,
											 u32 maxClients, const std::string& password,
											 const MetaData& hostMd, const MetaData& customMatchmakingMd ) override;

		MM_TS sptr<ISession> joinServer( const std::function<void( ISession&, bool )>& callback,
										 const IAddress& masterAddr, const std::string& name, const std::string& type,
										 float minRating, float maxRating, u32 minPlayers, u32 maxPlayers,
										 bool findP2p, bool findClientServer,
										 const MetaData& joinMd, const MetaData& customMatchmakingMd ) override;

		MM_TS bool kick( ILink& link ) override;
		MM_TS bool disconnect( ILink& link ) override;
		MM_TS bool disconnect( ISession& session ) override;
		MM_TS bool disconnectAll() override;

		MM_TS void createGroupInternal( const Session& session, const string& typeName, const BinSerializer& initData, byte channel, IDeliveryTrace* trace );
		MM_TS void destroyGroup( u32 groupId ) override;
		MM_TS void createRemoteGroup( const string& typeName, u32 netId, const BinSerializer& initData, const IAddress& etp );

		MM_TS ESendCallResult sendReliable(byte id, const ISession* session, ILink* exlOrSpecific, const BinSerializer* bs, u32 numSerializers,
										   bool buffer, bool relay, byte channel, IDeliveryTrace* trace) override;
		MM_TS ESendCallResult sendReliable(byte id, const ISession* session, Link* exlOrSpecific, const BinSerializer** bs, u32 numSerializers,
										   bool buffer, bool relay, bool systemBit,  byte channel, IDeliveryTrace* trace);
		MM_TS ESendCallResult sendReliable(const vector<sptr<const struct NormalSendPacket>>&, const ISession* session, Link* exlOrSpecific,
										   bool buffer, byte channel, IDeliveryTrace* trace);

		MM_TS void addSessionListener( ISession& session, ISessionListener* listener ) override;
		MM_TS void removeSessionListener( ISession& session, const ISessionListener* listener ) override;

		MM_TS void simulatePacketLoss( u32 percentage ) override;
		MM_TS u32  packetLossPercentage() const;
        MM_TS u32  nextSessionId();


		MM_TS static void printMemoryLeaks();

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
		MM_TS ESendCallResult callRpc2( Args... args, const Session* session, ILink* exclOrSpecific=nullptr, 
										bool localCall=false, bool buffer=false, bool relay=false, bool systemBit=true, 
										byte channel=0, IDeliveryTrace* trace=nullptr );

	private:
		atomic_uint m_PacketLossPercentage;
        atomic_uint m_NextSessionId;
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
	MM_TS ESendCallResult MiepMiep::Network::callRpc2(Args... args, const Session* session, ILink* exclOrSpecific,
													  bool localCall, bool buffer, bool relay, bool systemBit,
													  byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., *this, bs, localCall, channel);
		return priv_send_rpc( *this, T::rpcName(), bs, session, exclOrSpecific, buffer, relay, systemBit, channel, trace );
	}


	template <typename T>
	Network& toNetwork(T& t) 
	{ 
		return sc<Network&>(t);
	}

}
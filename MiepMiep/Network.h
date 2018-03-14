#pragma once

#include "Memory.h"
#include "Component.h"
#include "MiepMiep.h"
using namespace std;


namespace MiepMiep
{
	class Link;
	class Endpoint;

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
		Network(bool allowAsyncCallbacks);
		~Network() override;

		void processEvents() override;

		// INetwork
		MM_TS EListenCallResult startListen( u16 port, const std::string& pw="", u32 maxConnections=32 ) override;
		MM_TS bool stopListen( u16 port ) override;
		MM_TS ERegisterServerCallResult registerServer( const IEndpoint& masterEtp, const string& serverName, const string& pw="", const MetaData& md=MetaData() ) override;
		MM_TS EJoinServerCallResult joinServer( const IEndpoint& masterEtp, const string& serverName, const string& pw="", const MetaData& md=MetaData() ) override;

		MM_TS void createGroupInternal( const string& typeName, const BinSerializer& initData, byte channel, IDeliveryTrace* trace );
		MM_TS void destroyGroup( u32 groupId ) override;

		MM_TS void createRemoteGroup( const string& typeName, u32 netId, const BinSerializer& initData, const IEndpoint& etp );

		MM_TS ESendCallResult sendReliable(byte id, const BinSerializer* bs, u32 numSerializers, const IEndpoint* specific, 
										   bool exclude, bool buffer, bool relay,
										   byte channel, IDeliveryTrace* trace) override;
		MM_TS ESendCallResult sendReliable(byte id, const BinSerializer** bs, u32 numSerializers, const IEndpoint* specific, 
										   bool exclude, bool buffer, bool relay, bool systemBit,
										   byte channel, IDeliveryTrace* trace);
		MM_TS ESendCallResult sendReliable(const vector<sptr<const struct NormalSendPacket>>&, const IEndpoint* specific, 
										   bool exclude, bool buffer, byte channel, IDeliveryTrace* trace);

		MM_TS void addConnectionListener( IConnectionListener* listener ) override;
		MM_TS void removeConnectionListener( const IConnectionListener* listener ) override;


		MM_TS static void clearAllStatics();
		MM_TS MM_DECLSPEC static void printMemoryLeaks();

		// Gets, checks or adds directly in a link in the network -> linkManagerComponent.
		MM_TS sptr<Link> getLink(const Endpoint& etp) const;
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
		MM_TS ESendCallResult callRpc2( Args... args, bool localCall=false, const IEndpoint* specific=nullptr, bool exclude=false, bool buffer=false, 
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
	MM_TS ESendCallResult MiepMiep::Network::callRpc2(Args... args, bool localCall, const IEndpoint* specific, 
													  bool exclude, bool buffer, bool relay, bool systemBit,
													  byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., *this, bs, localCall);
		return priv_send_rpc( *this, T::rpcName(), bs, specific, exclude, buffer, relay, systemBit, channel, trace );
	}


	template <typename T>
	Network& toNetwork(T& t) 
	{ 
		return sc<Network&>(t);
	}

}
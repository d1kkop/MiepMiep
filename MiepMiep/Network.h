#pragma once

#include "MiepMiep.h"
#include "Memory.h"
#include "Component.h"
using namespace std;


namespace MiepMiep
{
	enum class ENetworkError
	{
		Fine,
		SerializationError,
		LogicalError
	};


	class Network: public ComponentCollection, public INetwork, public ITraceable
	{
	public:
		Network();
		~Network() override;

		// INetwork
		MM_TS ERegisterServerCallResult registerServer( const IEndpoint& masterEtp, const string& serverName, const string& pw="", const MetaData& md=MetaData() ) override;
		MM_TS EJoinServerCallResult joinServer( const IEndpoint& masterEtp, const string& serverName, const string& pw="", const MetaData& md=MetaData() ) override;

		MM_TS void registerRpc( u16 rpcType, const std::function<void (BinSerializer&, INetwork&, const IEndpoint*)>& cb ) override;
		MM_TS void deregisterRpc( u16 rpcType ) override;
		MM_TS void callRpc( u16 rpcType, const IEndpoint* specific, bool exclude, bool buffer ) override;

		MM_TS void registerGroup( u16 groupType, const std::function<void (INetwork&, const IEndpoint&)>& cb ) override;
		MM_TS void deregisterGroup( u16 groupType ) override;
		MM_TS void createGroup( u16 groupType ) override;
		MM_TS void destroyGroup( u32 groupId ) override;

		EListenCallResult listen( u16 port );
		EListenCallResult listenAsMasterHost( u16 port );


		template <typename T, typename ...Args> 
		MM_TS T* getOrAdd(byte idx=0, Args... args);


		MM_TS static void clearAllStatics();
		MM_TS MM_DECLSPEC static void printMemoryLeaks();

	private:
	};


	template <typename T, typename ...Args>
	MM_TS T* MiepMiep::Network::getOrAdd(byte idx, Args... args)
	{
		return getOrAddInternal<T, Network&>(idx, *this, args...);
	}
}
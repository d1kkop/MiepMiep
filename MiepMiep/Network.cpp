#include "Network.h"
#include "LinkState.h"
#include "LinkManager.h"
#include "Platform.h"
#include "GroupCollection.h"
#include "MsgIdToFunctions.h"
#include "PerThreadDataProvider.h"
#include "LinkState.h"


namespace MiepMiep
{
	// -------- INetwork -----------------------------------------------------------------------------------------------------

	MM_TS sptr<INetwork> INetwork::create()
	{
		if (0 == Platform::initialize())
		{
			return sptr<INetwork>(reserve<Network>(MM_FL));
		}
		return nullptr;
	}

	void INetwork::setLogSettings( bool logToFile, bool logToIde )
	{
		Platform::setLogSettings( logToFile, logToIde );
	}

	// -------- Network -----------------------------------------------------------------------------------------------------

	Network::Network()
	{
		// groups
		registerType<LinkState>();
		// rpc's
		//registerRpc<

	}

	Network::~Network()
	{
		if ( Platform::shutdown() ) 
		{
			Network::clearAllStatics();
		}
	}

	MM_TS ERegisterServerCallResult Network::registerServer(const IEndpoint& masterEtp, const string& name, const string& pw, const MetaData& md)
	{
		LinkManager* lm = getOrAdd<LinkManager>();
		sptr<Link> link = lm->addLink( masterEtp );
		if ( link )
		{
			return ERegisterServerCallResult::AlreadyRegistered;
		}
		auto& bs = link->beginSend();
		auto* ls = link->getOrAdd<LinkState>();
		ls->connect();
		return ERegisterServerCallResult::Fine;
	}

	MM_TS EJoinServerCallResult Network::joinServer(const IEndpoint& masterEtp, const string& name, const string& pw, const MetaData& md)
	{
		return EJoinServerCallResult::Fine;
	}

	MM_TS void Network::registerRpc(u16 rpcType, const std::function<void(BinSerializer&, INetwork&, const IEndpoint*)>& cb)
	{
		getOrAdd<MsgIdToFunctions>()->registerRpc( rpcType, cb );
	}

	MM_TS void Network::deregisterRpc(u16 rpcType)
	{
		getOrAdd<MsgIdToFunctions>()->deregisterRpc( rpcType );
	}

	MM_TS void Network::callRpc(u16 rpcType, const IEndpoint* specific, bool exclude, bool buffer)
	{
		// TODO buffer
		getOrAdd<MsgIdToFunctions>()->callRpc( rpcType, specific, exclude, buffer );
	}

	MM_TS void Network::registerGroup(u16 groupType, const std::function<void(INetwork&, const IEndpoint&)>& cb)
	{
		getOrAdd<MsgIdToFunctions>()->registerGroup( groupType, cb );
	}

	MM_TS void Network::deregisterGroup(u16 groupType)
	{
		getOrAdd<MsgIdToFunctions>()->deregisterGroup( groupType );
	}

	MM_TS void Network::createGroup(u16 groupType)
	{
		auto& vars = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionNetwork>()->addNewPendingGroup( vars, groupType ); // makes copy
		vars.clear();
	}

	MM_TS void Network::destroyGroup(u32 groupId)
	{

	}

	MM_TS void Network::clearAllStatics()
	{
		// TODO add more clear statics here
		PerThreadDataProvider::cleanupStatics();
	}

	MM_TS void Network::printMemoryLeaks()
	{
		Memory::printUntracedMemory();
	}

}
#include "Network.h"
#include "LinkState.h"
#include "LinkManager.h"
#include "Platform.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "LinkState.h"
#include "ReliableSend.h"
#include "JobSystem.h"
#include "NetworkListeners.h"


namespace MiepMiep
{
	// -------- INetwork -----------------------------------------------------------------------------------------------------

	MM_TS sptr<INetwork> INetwork::create()
	{
		if (0 == Platform::initialize())
		{
			return reserve_sp<Network>(MM_FL);
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
		getOrAdd<JobSystem>(0, 3); // 3 worker threads
	}

	Network::~Network()
	{
		if ( has<JobSystem>() )
		{
			get<JobSystem>()->stop();
		}
		if ( Platform::shutdown() ) 
		{
			Network::clearAllStatics();
		}
	}

	MM_TS void Network::processEvents()
	{
		getOrAdd<NetworkListeners>()->processAll();
	}

	MM_TS ERegisterServerCallResult Network::registerServer(const IEndpoint& masterEtp, const string& name, const string& pw, const MetaData& md)
	{
		sptr<LinkManager> lm = getOrAdd<LinkManager>();
		sptr<Link> link = lm->addLink( masterEtp );
		if ( link )
		{
			return ERegisterServerCallResult::AlreadyRegistered;
		}
		auto& bs = link->beginSend();
		sptr<LinkState> ls = link->getOrAdd<LinkState>();
		ls->connect();
		return ERegisterServerCallResult::Fine;
	}

	MM_TS EJoinServerCallResult Network::joinServer(const IEndpoint& masterEtp, const string& name, const string& pw, const MetaData& md)
	{
		return EJoinServerCallResult::Fine;
	}

	MM_TS void Network::createGroupInternal(const string& groupType, const BinSerializer& initData, byte channel, IDeliveryTrace* trace)
	{
		auto& vars = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionNetwork>(channel)->addNewPendingGroup( vars, groupType, initData, trace ); // makes copy
		vars.clear();
	}

	void priv_create_group(INetwork& nw, const char* groupType, BinSerializer& bs, byte channel, IDeliveryTrace* trace)
	{
		Network& network = static_cast<Network&>(nw);
		network.createGroupInternal( groupType, bs, channel, trace );
	}

	MM_TS void Network::destroyGroup(u32 groupId)
	{

	}

	MM_TS void Network::createRemoteGroup(const string& typeName, u32 netId, const BinSerializer& initData, const IEndpoint& etp)
	{
		void* vgCreatePtr = Platform::getPtrFromName((string("deserialize_") + typeName).c_str());
		if ( vgCreatePtr )
		{
			using fptr = void (*)(INetwork&, const IEndpoint&, const BinSerializer&);
			((fptr)vgCreatePtr)( *this, etp, initData );
		}
	}

	MM_TS ESendCallResult Network::sendReliable(BinSerializer& bs, const IEndpoint* specific,
												bool exclude, bool buffer, bool relay,
												byte channel, IDeliveryTrace* trace)
	{
		bool wasSent = getOrAdd<LinkManager>()->forLink( specific, exclude, [&](Link& link)
		{
			link.getOrAdd<ReliableSend>(channel)->enqueue( bs, relay, trace );
		});
		return wasSent?ESendCallResult::Fine:ESendCallResult::NotSent;
	}

	MM_TS void Network::addConnectionListener(IConnectionListener* listener)
	{
		getOrAdd<NetworkListeners>()->addListener<IConnectionListener>(listener);
	}

	MM_TS void Network::removeConnectionListener(const IConnectionListener* listener)
	{
		getOrAdd<NetworkListeners>()->removeListener<IConnectionListener>(listener);
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

	MM_TS sptr<Link> Network::getLink(const IEndpoint& etp) const
	{
		sptr<LinkManager> lm = get<LinkManager>();
		if (!lm) return nullptr;
		return lm->getLink(etp);
	}

}
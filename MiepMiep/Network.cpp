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
#include "MasterJoinData.h"
#include "PacketHandler.h"
#include "SendThread.h"
#include "SocketSetManager.h"


namespace MiepMiep
{
	// -------- INetwork -----------------------------------------------------------------------------------------------------

	MM_TS sptr<INetwork> INetwork::create(bool allowAsyncCalbacks)
	{
		if (0 == Platform::initialize())
		{
			return reserve_sp<Network>(MM_FL, allowAsyncCalbacks);
		}
		return nullptr;
	}

	void INetwork::setLogSettings( bool logToFile, bool logToIde )
	{
		Platform::setLogSettings( logToFile, logToIde );
	}

	// -------- Network -----------------------------------------------------------------------------------------------------

	Network::Network(bool allowAsyncCallbacks):
		m_AllowAsyncCallbacks(allowAsyncCallbacks)
	{
		getOrAdd<JobSystem>(0, 4);   // N worker threads
		getOrAdd<SendThread>();		 // starts a 'resend' flow and creates jobs per N links whom dispatch their data
		getOrAdd<SocketSetManager>(); // each N sockets is a new reception thread, default N = 64
	}

	Network::~Network()
	{
		get<JobSystem>()->stop();
		get<SendThread>()->stop();
		get<SocketSetManager>()->stop();
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
		get<JobSystem>()->addJob( [=]()
		{
			link->getOrAdd<MasterJoinData>()->setName( name );
			link->getOrAdd<MasterJoinData>()->setMetaData( md );
			link->getOrAdd<LinkState>()->connect( pw );
		});
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

	MM_TS ESendCallResult Network::sendReliable(byte id, const BinSerializer* bs, u32 numSerializers, const IEndpoint* specific, 
												bool exclude, bool buffer, bool relay, byte channel, IDeliveryTrace* trace)
	{
		const BinSerializer* bs2 [] = { bs } ;
		return sendReliable( id, bs2, numSerializers, specific, exclude, buffer, relay, false, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable(byte id, const BinSerializer** bs, u32 numSerializers, const IEndpoint* specific,
												bool exclude, bool buffer, bool relay, bool systemBit, byte channel, IDeliveryTrace* trace)
	{
		vector<sptr<const NormalSendPacket>> packets;
		if ( !PacketHelper::createNormalPacket( packets, (byte)ReliableSend::compType(), id, bs, numSerializers, channel, relay, systemBit,
			  MM_MAX_FRAGMENTSIZE /* TODO change for current known max MTU */ ))
		{
			return ESendCallResult::SerializationError;
		}
		return sendReliable( packets, specific, exclude, buffer, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable(const vector<sptr<const NormalSendPacket>>& data, const IEndpoint* specific,
												bool exclude, bool buffer, byte channel, IDeliveryTrace* trace)
	{
		bool wasSent = getOrAdd<LinkManager>()->forLink( specific, exclude, [&, data](Link& link)
		{
			link.getOrAdd<ReliableSend>(channel)->enqueue( data, trace );
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

	MM_TS sptr<Link> Network::getLink(const Endpoint& etp) const
	{
		sptr<LinkManager> lm = get<LinkManager>();
		if (!lm) return nullptr;
		return lm->getLink(etp);
	}

}
#include "Network.h"
#include "Link.h"
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
#include "Listener.h"
#include "Endpoint.h"
#include "Socket.h"


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

	MM_TS EListenCallResult Network::startListen(u16 port, const std::string& pw, u32 maxConnections)
	{
		sptr<Listener> listener;
		{
			scoped_lock lk(m_ListenerAddMutex);
			u32 i=0;
			while ( has<Listener>(i) ) i++;
			listener = getOrAdd<Listener>(i);
		}
		assert( listener );
		listener->setPassword( pw );
		listener->setMaxConnections( maxConnections );
		if ( !listener->startOrRestartListening( port )) 
		{
			return EListenCallResult::SocketError;
		}
		return EListenCallResult::Fine;
	}

	MM_TS bool Network::stopListen(u16 port)
	{
		sptr<Listener> listener;
		{
			scoped_lock lk(m_ListenerAddMutex);
			u32 s=0, e = count<Listener>();
			while (s < e)
			{
				sptr<Listener> listener = get<Listener>(s);
				if ( listener && listener->getPort() == port )
				{
					return remove<Listener>( s );
				}
				++s;
			}
		}
		return false;
	}

	MM_TS void Network::registerServer(const IAddress& masterAddr, const std::string& serverName, const std::string& pw,
									   const std::string& type, const MetaData& hostMd,
									   float initialRating, const MetaData& customFilterMd)
	{
		sptr<LinkManager> lm = getOrAdd<LinkManager>();
		sptr<Link> link = lm->add( masterAddr );
		get<JobSystem>()->addJob( [=]()
		{
			auto mjd = link->getOrAdd<MasterJoinData>(0, serverName, type, pw, initialRating);
			mjd->setServerProps( hostMd );
			link->getOrAdd<LinkState>()->connect(pw, customFilterMd);
		});
	}

	MM_TS EJoinServerCallResult Network::joinServer(const IAddress& masterAddr, const std::string& serverName, const std::string& pw,
													const std::string& type, const MetaData& joinMd, 
													float initialRating, float minRating, float maxRating,
													u32 minPlayers, u32 maxPlayers)
	{
		sptr<LinkManager> lm = getOrAdd<LinkManager>();
		sptr<Link> link = lm->add(masterAddr);
		get<JobSystem>()->addJob([=]()
		{
			auto mjd = link->getOrAdd<MasterJoinData>(0, serverName, type, pw, initialRating);
			mjd->setJoinFilter( joinMd, minPlayers, maxPlayers, minRating, maxRating );
			link->getOrAdd<LinkState>()->connect(pw, MetaData());
		});
		return EJoinServerCallResult::Fine;
	}

	MM_TS void Network::createGroupInternal(const ISender& sender, const string& groupType, const BinSerializer& initData, byte channel, IDeliveryTrace* trace)
	{
		auto& vars = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionNetwork>(channel)->addNewPendingGroup( sender, vars, groupType, initData, trace ); // makes copy
		vars.clear();
	}

	ECreateGroupCallResult priv_create_group(INetwork& nw, const char* groupType, BinSerializer& bs, byte channel, IDeliveryTrace* trace, const ISender* sender)
	{
		Network& network = static_cast<Network&>(nw);
		if ( sender )
		{
			network.createGroupInternal( *sender, groupType, bs, channel, trace );
			return ECreateGroupCallResult::Fine;
		}
		else
		{
			bool hasMultipleSenders;
			sptr<const ISender> sd = network.getFirstNetworkIdentity( hasMultipleSenders );
			if ( hasMultipleSenders )
			{
				// this is critical
				return ECreateGroupCallResult::MultipleLinksWhileNoSenderSpecified;
			}
			if ( sd )
			{
				network.createGroupInternal( *sd, groupType, bs, channel, trace );
				return ECreateGroupCallResult::Fine;
			}
			else
			{
				LOGW( "Cannot create network group as there are no links in the network (yet)." );
				return ECreateGroupCallResult::NoLinksInNetwork;
			}
		}
	}

	MM_TS void Network::destroyGroup(u32 groupId)
	{

	}

	MM_TS void Network::createRemoteGroup(const string& typeName, u32 netId, const BinSerializer& initData, const IAddress& etp)
	{
		void* vgCreatePtr = Platform::getPtrFromName((string("deserialize_") + typeName).c_str());
		if ( vgCreatePtr )
		{
			using fptr = void (*)(INetwork&, const IAddress&, const BinSerializer&);
			((fptr)vgCreatePtr)( *this, etp, initData );
		}
	}

	MM_TS ESendCallResult Network::sendReliable(const ISender& sender, byte id, const BinSerializer* bs, u32 numSerializers, const IAddress* specific, 
												bool exclude, bool buffer, bool relay, byte channel, IDeliveryTrace* trace)
	{
		const BinSerializer* bs2 [] = { bs } ;
		return sendReliable( sender, id, bs2, numSerializers, specific, exclude, buffer, relay, false, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable(const ISender& sender, byte id, const BinSerializer** bs, u32 numSerializers, const IAddress* specific,
												bool exclude, bool buffer, bool relay, bool systemBit, byte channel, IDeliveryTrace* trace)
	{
		vector<sptr<const NormalSendPacket>> packets;
		if ( !PacketHelper::createNormalPacket( packets, (byte)ReliableSend::compType(), id, bs, numSerializers, channel, relay, systemBit,
			  MM_MAX_FRAGMENTSIZE /* TODO change for current known max MTU */ ))
		{
			return ESendCallResult::SerializationError;
		}
		return sendReliable( sender, packets, specific, exclude, buffer, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable(const ISender& sender, const vector<sptr<const NormalSendPacket>>& data, const IAddress* specific,
												bool exclude, bool buffer, byte channel, IDeliveryTrace* trace)
	{
		bool wasSent = getOrAdd<LinkManager>()->forLink(sc<const ISocket&>(sender), specific, exclude, [&, data](Link& link)
		{
			link.getOrAdd<ReliableSend>(channel)->enqueue(data, trace);
		});
		return wasSent ? ESendCallResult::Fine : ESendCallResult::NotSent;
	}

	MM_TS void Network::addConnectionListener(IConnectionListener* listener)
	{
		getOrAdd<NetworkListeners>()->addListener<IConnectionListener>(listener);
	}

	MM_TS void Network::removeConnectionListener(const IConnectionListener* listener)
	{
		getOrAdd<NetworkListeners>()->removeListener<IConnectionListener>(listener);
	}

	MM_TS bool Network::isListenerSocket(const ISocket& sock) const
	{
		bool bfound = false;
		forAll<Listener>([&](const Listener& listener)
		{
			if ( listener.socket() == sock ) 
			{
				bfound = true;
				return false; // stop iterating
			}
			return true; // continue iterating
		});
		return bfound;
	}

	MM_TS sptr<const ISender> Network::getFirstNetworkIdentity(bool& hasMultiple) const
	{
		// Either return first from listeners or from links. This works only
		// in the general case of connecting to a single identity.
		auto l  = get<Listener>();
		auto lm = get<LinkManager>();
		auto fl = lm ? lm->getFirstLink() : nullptr;
		hasMultiple = false;
		if ( l && fl )
		{
			hasMultiple = true;
			LOGC( "Requesting first network identity while there are multiple in the network found." );
			assert( false );
		}
		if ( l )  return  l->socket().to_ptr();
		if ( fl ) return  fl->socket().to_ptr();
		return nullptr;
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

	MM_TS sptr<Link> Network::getLink(const SocketAddrPair& sap) const
	{
		sptr<LinkManager> lm = get<LinkManager>();
		if (!lm) return nullptr;
		return lm->get(sap);
	}

}
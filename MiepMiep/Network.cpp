#include "Network.h"
#include "Session.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "LinkState.h"
#include "ReliableSend.h"
#include "JobSystem.h"
#include "NetworkListeners.h"
#include "MasterLinkData.h"
#include "SendThread.h"
#include "SocketSetManager.h"
#include "ListenerManager.h"
#include "Listener.h"
#include "Socket.h"
#include "MasterServer.h"


namespace MiepMiep
{
	// -------- INetwork -----------------------------------------------------------------------------------------------------

	MM_TS sptr<INetwork> INetwork::create( bool allowAsyncCalbacks )
	{
		if ( 0 == Platform::initialize() )
		{
			return reserve_sp<Network>( MM_FL, allowAsyncCalbacks );
		}
		return nullptr;
	}

	void INetwork::setLogSettings( bool logToFile, bool logToIde )
	{
		Platform::setLogSettings( logToFile, logToIde );
	}

	// -------- Network -----------------------------------------------------------------------------------------------------

	Network::Network( bool allowAsyncCallbacks )
	{
		getOrAdd<JobSystem>( 0, 4 );   // N worker threads
		getOrAdd<SendThread>();		 // starts a 'resend' flow and creates jobs per N links whom dispatch their data
		getOrAdd<SocketSetManager>(); // each N sockets is a new reception thread, default N = 64
		getOrAdd<NetworkListeners>( 0, allowAsyncCallbacks );
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
		get<NetworkListeners>()->processAll();
	}

	MM_TS EListenCallResult Network::startListen( u16 port )
	{
		return getOrAdd<ListenerManager>()->startListen( port );
	}

	MM_TS void Network::stopListen( u16 port )
	{
		getOrAdd<ListenerManager>()->stopListen( port );
	}

	MM_TS bool Network::registerServer( const std::function<void( const ILink& link, bool )>& callback,
										const IAddress& masterAddr, bool isP2p, bool isPrivate, float rating,
										u32 maxClients, const std::string& name, const std::string& type, const std::string& password,
										const MetaData& hostMd, const MetaData& customMatchmakingMd )
	{
		auto link = getOrAdd<LinkManager>()->add( nullptr, masterAddr, true );	
		if ( !link ) return false;
		get<JobSystem>()->addJob( [=]
		{
			assert(link);
			auto s = reserve_sp<Session>( MM_FL, link, "", hostMd );
			link->setSession( s );
			MasterSessionData data;
			data.m_Type = type;
			data.m_Name = name;
			data.m_IsP2p = isP2p;
			data.m_Rating = rating;
			data.m_Password = password;
			data.m_IsPrivate = isPrivate;
			data.m_MaxClients = maxClients;
			link->getOrAdd<MasterLinkData>()->registerServer( callback, data, customMatchmakingMd );
		} );
		return true;
	}

	MM_TS bool Network::joinServer( const std::function<void( const ILink& link, EJoinServerResult )>& callback,
									const IAddress& masterAddr, const std::string& name, const std::string& type,
									float minRating, float maxRating, u32 minPlayers, float maxPlayers,
									bool findP2p, bool findClientServer,
									const MetaData& joinMd, const MetaData& customMatchmakingMd )
	{
		auto link = getOrAdd<LinkManager>()->add( nullptr, masterAddr, true );
		if ( !link ) return false;
		get<JobSystem>()->addJob( [=]
		{
			assert(link);
			auto s = reserve_sp<Session>( MM_FL, link, "", joinMd );
			link->setSession( s );
			SearchFilter sf;
			sf.m_Name = name;
			sf.m_Type = type;
			sf.m_MinRating = minRating;
			sf.m_MaxRating = maxRating;
			sf.m_FindPrivate = false;
			sf.m_FindP2p = findP2p;
			sf.m_FindClientServer = findClientServer;
			link->getOrAdd<MasterLinkData>()->joinServer( callback, sf, customMatchmakingMd );
		} );
		return true;
	}

	MM_TS void Network::createGroupInternal( const ISender& sender, const string& groupType, const BinSerializer& initData, byte channel, IDeliveryTrace* trace )
	{
		auto& vars = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionNetwork>( channel )->addNewPendingGroup( sender, vars, groupType, initData, trace ); // makes copy
		vars.clear();
	}

	ECreateGroupCallResult priv_create_group( INetwork& nw, const char* groupType, BinSerializer& bs, byte channel, IDeliveryTrace* trace, const ISender* sender )
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

	MM_TS void Network::destroyGroup( u32 groupId )
	{

	}

	MM_TS void Network::createRemoteGroup( const string& typeName, u32 netId, const BinSerializer& initData, const IAddress& etp )
	{
		void* vgCreatePtr = Platform::getPtrFromName( (string( "deserialize_" ) + typeName).c_str() );
		if ( vgCreatePtr )
		{
			using fptr = void( *)(INetwork&, const IAddress&, const BinSerializer&);
			((fptr)vgCreatePtr)(*this, etp, initData);
		}
	}

	MM_TS ESendCallResult Network::sendReliable( const ISender& sender, byte id, const BinSerializer* bs, u32 numSerializers, const IAddress* specific,
												 bool exclude, bool buffer, bool relay, byte channel, IDeliveryTrace* trace )
	{
		const BinSerializer* bs2[] = { bs };
		return sendReliable( sender, id, bs2, numSerializers, specific, exclude, buffer, relay, false, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable( const ISender& sender, byte id, const BinSerializer** bs, u32 numSerializers, const IAddress* specific,
												 bool exclude, bool buffer, bool relay, bool systemBit, byte channel, IDeliveryTrace* trace )
	{
		vector<sptr<const NormalSendPacket>> packets;
		if ( !PacketHelper::createNormalPacket( packets, (byte)ReliableSend::compType(), id, bs, numSerializers, channel, relay, systemBit,
			 MM_MAX_FRAGMENTSIZE /* TODO change for current known max MTU */ ) )
		{
			return ESendCallResult::SerializationError;
		}
		return sendReliable( sender, packets, specific, exclude, buffer, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable( const ISender& sender, const vector<sptr<const NormalSendPacket>>& data, const IAddress* specific,
												 bool exclude, bool buffer, byte channel, IDeliveryTrace* trace )
	{
		bool wasSent = getOrAdd<LinkManager>()->forLink( sc<const ISocket&>( sender ), specific, exclude, [&, data] ( Link& link )
		{
			link.getOrAdd<ReliableSend>( channel )->enqueue( data, trace );
		} );
		return wasSent ? ESendCallResult::Fine : ESendCallResult::NotSent;
	}

	MM_TS void Network::addConnectionListener( IConnectionListener* listener )
	{
		get<NetworkListeners>()->addListener<IConnectionListener>( listener );
	}

	MM_TS void Network::removeConnectionListener( const IConnectionListener* listener )
	{
		get<NetworkListeners>()->removeListener<IConnectionListener>( listener );
	}

	MM_TS sptr<const ISender> Network::getFirstNetworkIdentity( bool& hasMultiple ) const
	{
		// Either return first from listeners or from links. This works only
		// in the general case of connecting to a single identity.
		auto l = get<Listener>();
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

	MM_TS sptr<Link> Network::getLink( const SocketAddrPair& sap ) const
	{
		sptr<LinkManager> lm = get<LinkManager>();
		if ( !lm ) return nullptr;
		return lm->get( sap );
	}

}
#include "Network.h"
#include "Session.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "LinkState.h"
#include "ReliableSend.h"
#include "JobSystem.h"
#include "NetworkEvents.h"
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
		getOrAdd<NetworkEvents>( 0, allowAsyncCallbacks );
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
		get<NetworkEvents>()->processQueuedEvents();
	}

	MM_TS EListenCallResult Network::startListen( u16 port )
	{
		return getOrAdd<ListenerManager>()->startListen( port );
	}

	MM_TS void Network::stopListen( u16 port )
	{
		getOrAdd<ListenerManager>()->stopListen( port );
	}

	MM_TS bool Network::registerServer( const std::function<void( const ISession&, bool )>& callback,
										const IAddress& masterAddr, bool isP2p, bool isPrivate, bool canJoinAfterStart, float rating,
										u32 maxClients, const std::string& name, const std::string& type, const std::string& password,
										const MetaData& hostMd, const MetaData& customMatchmakingMd )
	{
		i32 err;
		auto sock = ISocket::create( 0, &err );
		if ( !sock )
		{
			LOGW( "Could not create socket, error %d.", err );
			return false;
		}
		auto s = reserve_sp<Session>( MM_FL, *this, hostMd );
		auto link = getOrAdd<LinkManager>()->add( *s, SocketAddrPair( *sock, masterAddr ), true );	
		if ( !link ) return false;
		get<JobSystem>()->addJob( [=]
		{
			assert(link && s);
			s->setMasterLink( link );
			MasterSessionData data;
			data.m_Type = type;
			data.m_Name = name;
			data.m_IsP2p = isP2p;
			data.m_Rating = rating;
			data.m_Password = password;
			data.m_IsPrivate  = isPrivate;
			data.m_MaxClients = maxClients;
			data.m_UsedMatchmaker = true;
			data.m_CanJoinAfterStart = canJoinAfterStart;
			link->getOrAdd<MasterLinkData>()->registerServer( callback, data, customMatchmakingMd );
		} );
		return true;
	}

	MM_TS bool Network::joinServer( const std::function<void( const ISession&, EJoinServerResult )>& callback,
									const IAddress& masterAddr, const std::string& name, const std::string& type,
									float minRating, float maxRating, u32 minPlayers, u32 maxPlayers,
									bool findP2p, bool findClientServer,
									const MetaData& joinMd, const MetaData& customMatchmakingMd )
	{
		i32 err;
		auto sock = ISocket::create( 0, &err );
		if ( !sock )
		{
			LOGW( "Could not create socket, error %d.", err );
			return false;
		}
		auto s = reserve_sp<Session>( MM_FL, *this, joinMd );
		auto link = getOrAdd<LinkManager>()->add( *s, SocketAddrPair( *sock, masterAddr ), true );
		if ( !link ) return false;
		get<JobSystem>()->addJob( [=]
		{
			assert(link && s);
			s->setMasterLink( link );
			SearchFilter sf;
			sf.m_Name = name;
			sf.m_Type = type;
			sf.m_MinRating   = minRating;
			sf.m_MaxRating   = maxRating;
			sf.m_MinPlayers  = minPlayers;
			sf.m_MaxPlayers  = maxPlayers;
			sf.m_FindPrivate = false;
			sf.m_FindP2p = findP2p;
			sf.m_FindClientServer = findClientServer;
			link->getOrAdd<MasterLinkData>()->joinServer( callback, sf, customMatchmakingMd );
		} );
		return true;
	}

	MM_TS bool Network::kick( ILink& link )
	{
		return sc<Link&>( link ).disconnect( true, true );
	}

	MM_TS bool Network::disconnect( ILink& link )
	{
		return sc<Link&>(link).disconnect( false, true );
	}

	MM_TS bool Network::disconnect( ISession& session )
	{
		return sc<Session&>(session).disconnect();
	}

	MM_TS bool Network::disconnectAll()
	{
		auto lm = get<LinkManager>();
		if ( lm )
		{
			lm->forEachLink( [&] ( Link& link )
			{
				link.disconnect( false, true );
			}, 128 );
		}
		return true; // TODO , remove true or check num that disconnected
	}

	MM_TS void Network::createGroupInternal( const Session& session, const string& groupType, const BinSerializer& initData, byte channel, IDeliveryTrace* trace )
	{
		auto& vars = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionNetwork>( channel )->addNewPendingGroup( session, vars, groupType, initData, trace ); // makes copy
		vars.clear(); // <<-- Note per thread vars, so thread safe.
	}

	MM_TS ECreateGroupCallResult priv_create_group( INetwork& nw, const ISession& session, const char* groupType, BinSerializer& bs, byte channel, IDeliveryTrace* trace )
	{
		Network& network = sc<Network&>(nw);
		network.createGroupInternal( sc<const Session&>( session ), groupType, bs, channel, trace );
		return ECreateGroupCallResult::Fine;
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

	MM_TS ESendCallResult Network::sendReliable( byte id, const ISession* session, ILink* exlOrSpecific, const BinSerializer* bs, u32 numSerializers,
												 bool buffer, bool relay, byte channel, IDeliveryTrace* trace )
	{
		const BinSerializer* bs2[] = { bs };
		return sendReliable( id, session, sc<Link*>( exlOrSpecific ), bs2, numSerializers, buffer, relay, false, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable( byte id, const ISession* session, Link* exlOrSpecific, const BinSerializer** bs, u32 numSerializers,
												 bool buffer, bool relay, bool systemBit, byte channel, IDeliveryTrace* trace )
	{
		vector<sptr<const NormalSendPacket>> packets;
		if ( !PacketHelper::createNormalPacket( packets, (byte)ReliableSend::compType(), id, bs, numSerializers, channel, relay, systemBit,
			 MM_MAX_FRAGMENTSIZE /* TODO change for current known max MTU */ ) )
		{
			return ESendCallResult::SerializationError;
		}
		return sendReliable( packets, session, exlOrSpecific, buffer, channel, trace );
	}

	MM_TS ESendCallResult Network::sendReliable( const vector<sptr<const NormalSendPacket>>& data, const ISession* session, Link* exlOrSpecific,
												 bool buffer, byte channel, IDeliveryTrace* trace )
	{
		const Session* ses = sc<const Session*>(session);
		bool somethingWasQueued = false;
		if ( ses )
		{
			ses->forLink( exlOrSpecific, [&] ( Link& link )
			{
				link.getOrAdd<ReliableSend>( channel )->enqueue( data, trace );
				somethingWasQueued = true;
			});
		}
		else if ( exlOrSpecific )
		{
			exlOrSpecific->getOrAdd<ReliableSend>( channel )->enqueue( data, trace );
			somethingWasQueued = true;
		}
		else
		{
			LOGW( "Nothing was send, though a reliable call was made." );
		}
		return (somethingWasQueued ? ESendCallResult::Fine : ESendCallResult::NotSent );
	}

	MM_TS void Network::addSessionListener( ISession& session, ISessionListener* listener )
	{
	//	get<NetworkListeners>()->addListener<ISessionListener>( listener );
	}

	MM_TS void Network::removeSessionListener( ISession& session, const ISessionListener* listener )
	{
	//	get<NetworkListeners>()->removeListener<ISessionListener>( listener );
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
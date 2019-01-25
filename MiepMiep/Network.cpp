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
#include "Util.h"


namespace MiepMiep
{
	// -------- INetwork -----------------------------------------------------------------------------------------------------

	MM_TS sptr<INetwork> INetwork::create( bool allowAsyncCalbacks, u32 numWorkerThreads )
	{
		if ( 0 == Platform::initialize() )
		{
			return reserve_sp<Network>( MM_FL, allowAsyncCalbacks, numWorkerThreads );
		}
		return nullptr;
	}

	void INetwork::setLogSettings( bool logToFile, bool logToIde )
	{
		Platform::setLogSettings( logToFile, logToIde );
	}

	// -------- Network -----------------------------------------------------------------------------------------------------

	Network::Network( bool allowAsyncCallbacks, u32 numWorkerThreads )
	{
		if ( numWorkerThreads == 0 ) throw;
		getOrAdd<JobSystem>( 0, numWorkerThreads ); // N worker threads
		getOrAdd<SendThread>();		 // starts a 'resend' flow and creates jobs per N links whom dispatch their data
		getOrAdd<SocketSetManager>(); // each N sockets is a new reception thread, default N = 64
		getOrAdd<NetworkEvents>( 0, allowAsyncCallbacks );
	}

	Network::~Network()
	{
		get<SendThread>()->stop();
		get<SocketSetManager>()->stop();
		get<JobSystem>()->stop();
		Platform::shutdown();

		// Use this instead of m_Components.clear because that will invoke ~Link which will try to access
		// the map again to deregister itself from a SocketSetManager while the 'clear' function of the map is not finished.
		// Doing so results in crash in std::map.
		/* DONT USE --> m_Components.clear() <-- DONT USE */
		for ( auto& c : m_Components )
		{
			c.second.clear();
		}
	}

	MM_TS void Network::processEvents()
	{
		auto ne = get<NetworkEvents>();
		if ( ne ) ne->processQueuedEvents();
		else { LOGW( "Attempted to process events while NetworkEvents was destroyed." ); }
	}

	MM_TS EListenCallResult Network::startListen( u16 port )
	{
		return getOrAdd<ListenerManager>()->startListen( port );
	}

	MM_TS void Network::stopListen( u16 port )
	{
		getOrAdd<ListenerManager>()->stopListen( port );
	}

	MM_TS sptr<ISession> Network::registerServer( const std::function<void( ISession&, bool )>& callback,
												  const IAddress& masterAddr, const std::string& name, const std::string& type,
												  bool isP2p, bool canJoinAfterStart, float rating,
												  u32 maxClients, const std::string& password,
												  const MetaData& hostMd, const MetaData& customMatchmakingMd )
	{
		i32 err;
		auto sock = ISocket::create( 0, &err );
		if ( !sock )
		{
			LOGW( "Could not create socket, error %d.", err );
			return nullptr;
		}
        LOG("Registering a new server at master %s.", masterAddr.toIpAndPort());
		auto s = reserve_sp<Session>( MM_FL, *this, hostMd );
		auto link = getOrAdd<LinkManager>()->add( *s, SocketAddrPair( *sock, masterAddr ) );
		if ( !link ) return nullptr;
        assert(link && s);
        s->setMasterLink(link);
        MasterSessionData data;
        data.m_Type = type;
        data.m_Name = name;
        data.m_IsP2p  = isP2p;
        data.m_Rating = rating;
        data.m_Password = password;
        data.m_IsPrivate  = !password.empty();
        data.m_MaxClients = maxClients;
        data.m_UsedMatchmaker = true;
        data.m_CanJoinAfterStart = canJoinAfterStart;
        bool bStartedRegister = link->getOrAdd<MasterLinkData>()->registerServer(callback, data, customMatchmakingMd);
        assert(bStartedRegister);
		return s;
	}

	MM_TS sptr<ISession> Network::joinServer( const std::function<void( ISession&, bool )>& callback,
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
			return nullptr;
		}
        LOG("Joining a new server at master %s.", masterAddr.toIpAndPort()); 
		auto s = reserve_sp<Session>( MM_FL, *this, joinMd );
		auto link = getOrAdd<LinkManager>()->add( *s, SocketAddrPair( *sock, masterAddr ) );
		if ( !link ) return nullptr;
        assert(link && s);
        s->setMasterLink(link);
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
        bool bStartedJoin = link->getOrAdd<MasterLinkData>()->joinServer(callback, sf, customMatchmakingMd);
        assert(bStartedJoin);
		return s;
	}

	MM_TS bool Network::kick( ILink& link )
	{
		return sc<Link&>( link ).disconnect( Is_Kick, No_Receive, Remove_Link );
	}

	MM_TS bool Network::disconnect( ILink& link )
	{
		return sc<Link&>(link).disconnect( No_Kick, No_Receive, Remove_Link );
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
				link.disconnect( No_Kick, No_Receive, Remove_Link );
			}, 128 );
		}
		return true; // TODO , remove true or check num that disconnected
	}

	MM_TS void Network::createGroupInternal( const Session& session, const string& groupType, const BinSerializer& initData, byte channel, IDeliveryTrace* trace )
	{
		auto& vars = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionNetwork>( channel )->addNewPendingGroup( session, vars, groupType, initData, trace ); // makes copy
		vars.clear(); // <<-- Per thread vars, so thread safe.
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
		Session* ses = const_cast<Session*>( sc<const Session*>(session) );
		bool somethingWasQueued = false;
		if ( ses )
		{
			// Make buffering and sending an atomic operation to avoid discrepanties between adding new links and messages
			// and sending existing messages to new links.
			rscoped_lock lk( ses->dataMutex() );
			if ( buffer )
			{
				ses->bufferMsg( data, channel );
			}
			ses->forLink( exlOrSpecific, [&] ( Link& link )
			{
				link.getOrAdd<ReliableSend>( channel )->enqueue( data, trace );
				somethingWasQueued = true;
			});
		}
		else if ( exlOrSpecific )
		{
			assert( !buffer ); // Buffer is not valid as the buffered packet is stored at the session which is not available.
			exlOrSpecific->getOrAdd<ReliableSend>( channel )->enqueue( data, trace );
			somethingWasQueued = true;
		}
		if ( !somethingWasQueued )
		{
			LOG( "Nothing was sent, though a reliable call was made. Either 'session' or 'exlOrSpecific' must not be NULL." );
		}
		return (somethingWasQueued ? ESendCallResult::Fine : ESendCallResult::NotSent );
	}

	MM_TS void Network::addSessionListener( ISession& session, ISessionListener* listener )
	{
		sc<SessionBase&>(session).addListener( listener );
	}

	MM_TS void Network::removeSessionListener( ISession& session, const ISessionListener* listener )
	{
		sc<SessionBase&>(session).removeListener( listener );
	}

	MM_TS void Network::simulatePacketLoss( u32 percentage )
	{
		m_PacketLossPercentage = percentage;
	}

	MM_TS u32 Network::packetLossPercentage() const
	{
		return m_PacketLossPercentage;
	}

    MM_TS u32 Network::nextSessionId()
    {
        return m_NextSessionId++;
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
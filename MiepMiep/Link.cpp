#include "Link.h"
#include "Session.h"
#include "Network.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "Endpoint.h"
#include "Socket.h"
#include "LinkState.h"
#include "PacketHandler.h"
#include "ReliableRecv.h"
#include "UnreliableRecv.h"
#include "ReliableNewRecv.h"
#include "ReliableAckRecv.h"
#include "ReliableNewestAckRecv.h"
#include "SocketSetManager.h"
#include "Util.h"


namespace MiepMiep
{
	// -------- ILink ----------------------------------------------------------------------------------------------------

	MM_TS sptr<ILink> ILink::to_ptr()
	{
		return sc<Link&>(*this).ptr<Link>();
	}

	MM_TS sptr<const ILink> ILink::to_ptr() const
	{
		return sc<const Link&>(*this).ptr<const Link>();
	}

	// -------- Link ----------------------------------------------------------------------------------------------------

	Link::Link(Network& network):
		IPacketHandler(network),
		m_SocketWasAddedToHandler(false)
	{
	}

	Link::~Link()
	{
		if ( m_SocketWasAddedToHandler )
		{
			sptr<SocketSetManager> ss = m_Network.get<SocketSetManager>();
			if ( ss )
			{
				ss->removeSocket( m_Socket );
			}
		}
	}

	void Link::setSession( const sptr<Session>& session )
	{
		assert( !m_Session );
		m_Session = session;
	}

	bool Link::operator<( const Link& o ) const
	{
		return getSocketAddrPair() < o.getSocketAddrPair();
	}

	bool Link::operator==( const Link& o ) const
	{
		return getSocketAddrPair() == o.getSocketAddrPair();
	}

	void Link::setMasterSession( const sptr<MasterSession>& session )
	{
		scoped_spinlock lk(m_MasterSessionMutex);
		assert( !m_Session && !m_MasterSession );
		m_MasterSession = session;
	}

	MM_TS sptr<Link> Link::create( Network& network, const Session* session, const IAddress& destination, bool addHandler )
	{
		return create( network, session, destination, rand(), addHandler );
	}

	MM_TS sptr<Link> Link::create(Network& network, const Session* session, const IAddress& destination, u32 id, bool addHandler)
	{
		sptr<ISocket> sock = ISocket::create();
		if (!sock) return nullptr;

		i32 err;
		sock->open(IPProto::Ipv4, SocketOptions(), &err);
		if (err != 0)
		{
			LOGW("Socket open error %d, cannot create link.", err);
			return nullptr;
		}

		sock->bind(0, &err);
		if (err != 0)
		{
			LOGW("Socket bind error %d, cannot create link.", err);
			return nullptr;
		}

		return create( network, session, SocketAddrPair( *sock, destination ), id, addHandler );
	}

	MM_TS sptr<Link> Link::create( Network& network, const Session* session, const SocketAddrPair& sap, u32 id, bool addHandler )
	{
		assert( sap.m_Address && sap.m_Socket );
		if ( !sap.m_Address || !sap.m_Socket )
		{
			LOGC( "Invalid socket address pair provided. Link creation discarded." );
			return nullptr;
		}

		sptr<Link> link = reserve_sp<Link, Network&>( MM_FL, network );

		i32 err;
		link->m_Source = Endpoint::createSource( *sap.m_Socket, &err );
		if ( err != 0 )
		{
			LOGW( "Failed to obtain locally bound port.", err );
			return nullptr;
		}

		if ( addHandler )
		{
			network.getOrAdd<SocketSetManager>()->addSocket( sap.m_Socket, link->ptr<Link>() );
			link->m_SocketWasAddedToHandler = true;
		}

		link->m_Id = id;
		link->m_Socket = sap.m_Socket->to_sptr();
		link->m_Destination = sap.m_Address->to_ptr();
		link->m_Session = (session ? session->ptr<Session>() : nullptr);

		LOG( "Created new link to %s.", link->info() );
		return link;
	}

	MM_TS INetwork& Link::network() const
	{
		return m_Network;
	}

	MM_TS ISession* Link::session() const
	{
		return m_Session.get();
	}

	MM_TS MasterSession* Link::masterSession() const
	{
		scoped_spinlock lk(m_MasterSessionMutex);
		return m_MasterSession.get();
	}

	MM_TS bool Link::isConnected() const
	{
		auto ls = get<LinkState>();
		return ls && ls->state() == ELinkState::Connected;
	}

	MM_TS const char* Link::ipAndPort() const
	{
		return m_Destination->toIpAndPort();
	}

	MM_TS const char* Link::info() const
	{
		static thread_local char buff[128];
		Platform::formatPrint(buff, sizeof(buff), "%s (src_port: %d, link: %d, socket: %d)", ipAndPort(), source().port(), m_Id, socket().id());
		return buff;
	}

	MM_TS SocketAddrPair Link::getSocketAddrPair() const
	{
		return SocketAddrPair( *m_Socket, *m_Destination );
	}

	MM_TS void Link::updateCustomMatchmakingMd( const MetaData& md )
	{
		scoped_spinlock lk( m_MatchmakingDataMutex);
		m_CustomMatchmakingMd = md;
	}

	MM_TS void Link::createGroup( const string& typeName, const BinSerializer& initData )
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionLink>()->addNewPendingGroup( socket(), varVec, typeName, initData, nullptr ); // makes copy
		varVec.clear();
	}

	MM_TS void Link::destroyGroup(u32 id)
	{
		
	}

	MM_TS void Link::receive(BinSerializer& bs)
	{
		byte compType;
		PacketInfo pi;

		__CHECKED( bs.read(pi.m_Sequence) );
		__CHECKED( bs.read(compType) );
		__CHECKED( bs.read(pi.m_ChannelAndFlags) );

		byte channel = pi.m_ChannelAndFlags & MM_CHANNEL_MASK;
		EComponentType et = (EComponentType) compType;

		switch ( et )
		{
		case EComponentType::ReliableSend:
			getOrAdd<ReliableRecv>(channel)->receive( bs, pi );
			break;

		case EComponentType::UnreliableSend:
			getOrAdd<UnreliableRecv>(channel)->receive( bs );
			break;

		case EComponentType::ReliableNewSend:
			getOrAdd<ReliableNewRecv>()->receive( bs );
			break;

			// -- Acks --

		case EComponentType::ReliableAckSend:
			getOrAdd<ReliableAckRecv>()->receive( bs );
			break;

		case EComponentType::ReliableNewAckSend:
			getOrAdd<ReliableNewestAckRecv>()->receive( bs );
			break;

		default:
			LOGW( "Unknown stream type %d detected. Packet ignored.", (u32)et );
			break;
		}
	}

	MM_TS void Link::send(const byte* data, u32 length)
	{
		if ( !m_Socket )
		{
			LOGC( "Invalid socket, cannot send." );
			return;
		}

		i32 err = 0;
		ESendResult res = m_Socket->send( sc<const Endpoint&>( *m_Destination ), data, length, &err );
	
	//	thread_local static u32 kt=0;	
	//	LOG( "Send ... %d", kt++ );

		if ( err != 0 && ESendResult::Error==res ) /* ignore err if socket gets closed */
		{
			LOGW( "Socket send error %d.", err );
		}
	}

	MM_TO_PTR_IMP( Link )
}
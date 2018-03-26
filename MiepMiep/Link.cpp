#include "Link.h"
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

	MM_TS sptr<Link> Link::create(Network& network, const IAddress& destination, bool addHandler)
	{
		return create( network, destination, rand(), addHandler );
	}

	MM_TS sptr<Link> Link::create(Network& network, const IAddress& destination, u32 id, bool addHandler)
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

		return create( network, SocketAddrPair( *sock, destination ), id, addHandler );
	}

	MM_TS sptr<Link> Link::create( Network& network, const SocketAddrPair& sap, u32 id, bool addHandler )
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

		LOG( "Created new link to %s.", link->info() );
		return link;
	}

	MM_TS INetwork& Link::network() const
	{
		return m_Network;
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

	MM_TS void Link::createGroup(const string& typeName, const BinSerializer& initData)
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
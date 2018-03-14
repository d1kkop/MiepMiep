#include "Link.h"
#include "Network.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "Listener.h"
#include "Endpoint.h"
#include "Socket.h"
#include "LinkState.h"
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
		IPacketHandler(network)
	{
	}

	Link::~Link()
	{
		if ( m_Originator ) 
		{
			// NOTE: Do not remove socket from socket set, as socket was borroed from originator (listener).
			const_pointer_cast<Listener>( m_Originator )->reduceNumClientsByOne();
		}
		else
		{
			// Remove socket from set, if socket set manager is still alive (might be closing).
			auto ss = m_Network.get<SocketSetManager>();
			if ( ss )
			{
				ss->removeSocket( m_Socket );
			}
		}
	}

	sptr<Link> Link::create(Network& network, const IAddress& destination, u32* id, const Listener* originator)
	{
		assert( !(id || originator) || (id && originator) );

		// If called from 'connect' call, we initiate a new socket on a 'random' port
		// else, a socket is obtained from the originator (listener).
		sptr<ISocket> sock;
		sptr<IAddress> source;
		if ( id == nullptr )
		{
			sock = ISocket::create();
			if (!sock) return nullptr;

			i32 err;
			sock->open(IPProto::Ipv4, SocketOptions(), &err);
			if ( err != 0 )
			{
				LOGW( "Socket open error %d, cannot create link.", err );
				return nullptr;
			}

			sock->bind(0, &err);
			if ( err != 0 )
			{
				LOGW( "Socket bind error %d, cannot create link.", err );
				return nullptr;
			}

			// Now try obtain local source address
			source = Endpoint::createSource( *sock, &err );
			if ( err != 0 )
			{
				LOGW( "Could not obtain local address from socket. Error: %d.", err );
				return nullptr;
			}
		}

		sptr<Link> link = reserve_sp<Link, Network&>(MM_FL, network);
		link->m_Destination = destination.getCopy();
		if ( id ) // Created from 'listen'
		{
			link->m_Id = *id;
			link->m_Socket = originator->socket().to_ptr();
			link->m_Source = originator->source().to_ptr();
			link->m_Originator = originator->to_ptr();
		}
		else // Created from 'connect'
		{
			link->m_Id = rand();
			link->m_Socket = sock;
			link->m_Source = Endpoint::createSource( *sock );
			// add socket to set and receive data directly in the link
			link->m_Network.getOrAdd<SocketSetManager>()->addSocket( link->m_Socket, link->to_ptr() );
		}

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
		Platform::formatPrint(buff, sizeof(buff), "%s (Id: %d)", ipAndPort(), m_Id);
		return buff;
	}

	MM_TS SocketAddrPair Link::extractSocketAddrPair() const
	{
		return SocketAddrPair( *m_Socket, *m_Destination );
	}

	MM_TS void Link::createGroup(const string& typeName, const BinSerializer& initData)
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionLink>()->addNewPendingGroup( varVec, typeName, initData, nullptr ); // makes copy
		varVec.clear();
	}

	MM_TS void Link::destroyGroup(u32 id)
	{
		
	}

	MM_TS void Link::handleSpecial(class BinSerializer& bs, const class Endpoint& etp)
	{
		u32 linkId;
		__CHECKED( bs.read(linkId) );

		if ( m_Id == linkId )
		{
			receive( bs );
		}
		else
		{
			LOGW( "Packet for link was discarded as link id's did not match." );
		}
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
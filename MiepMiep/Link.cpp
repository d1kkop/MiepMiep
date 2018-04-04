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
#include "ReliableAckSend.h"
#include "ReliableAckRecv.h"
#include "SocketSetManager.h"
#include "MasterSession.h"
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
				ss->removeSocket( m_SockAddrPair.m_Socket );
			}
		}
		LOG( "Link %s destroyed.", info() );
	}

	MM_TSC bool Link::setSession( SessionBase& session )
	{
		assert( !m_Session );
		if ( session.addLink( to_ptr() ) )
		{
			m_Session = session.to_ptr();
			return true;
		}
		return false;
	}

	MM_TSC bool Link::operator<( const Link& o ) const
	{
		return getSocketAddrPair() < o.getSocketAddrPair();
	}

	MM_TSC bool Link::operator==( const Link& o ) const
	{
		return getSocketAddrPair() == o.getSocketAddrPair();
	}

	MM_TS sptr<Link> Link::create( Network& network, SessionBase& session, const SocketAddrPair& sap, bool addHandler )
	{
		return create( network, session, sap, Util::rand(), addHandler );
	}

	MM_TS sptr<Link> Link::create( Network& network, SessionBase& session, const SocketAddrPair& sap, u32 id, bool addHandler )
	{
		return create( network, &session, sap, id, addHandler );
	}

	MM_TS sptr<Link> Link::create( Network& network, SessionBase* session, const SocketAddrPair& sap, u32 id, bool addHandler )
	{
		sptr<Link> link = reserve_sp<Link, Network&>( MM_FL, network );

		i32 err=0;
		link->m_Source = Endpoint::createSource( *sap.m_Socket, &err );
		if ( err != 0 )
		{
			LOGW( "Local endpoint could not be resolved, error %d.", err );
			return nullptr;
		}

		if ( addHandler )
		{
			link->m_Network.getOrAdd<SocketSetManager>()->addSocket( sap.m_Socket, link->ptr<IPacketHandler>() );
			link->m_SocketWasAddedToHandler = true;
		}

		link->m_Id = id;
		link->m_SockAddrPair = sap;
		if ( session )
		{
			if ( session->addLink( link ) )
			{
				link->m_Session = session->to_ptr();
			}
			else
			{
				return nullptr;
			}
		}

		LOG( "Created new link to %s.", link->info() );
		return link;
	}

	MM_TSC INetwork& Link::network() const
	{
		return m_Network;
	}

	MM_TSC ISession& Link::session() const
	{
		assert(m_Session);
		return *m_Session;
	}

	MM_TS bool Link::isConnected() const
	{
		auto ls = get<LinkState>();
		return ls && ls->state() == ELinkState::Connected;
	}

	MM_TSC SessionBase* Link::getSession() const
	{
		return m_Session.get();
	}

	MM_TSC const char* Link::ipAndPort() const
	{
		assert(m_SockAddrPair.m_Address);
		return m_SockAddrPair.m_Address->toIpAndPort();
	}

	MM_TSC const char* Link::info() const
	{
		static thread_local char buff[128];
		Platform::formatPrint(buff, sizeof(buff), "%s (src_port: %d, link: %d, socket: %d)", ipAndPort(), source().port(), m_Id, socket().id());
		return buff;
	}

	MM_TSC const SocketAddrPair& Link::getSocketAddrPair() const
	{
		return m_SockAddrPair;
	}

	MM_TSC Session& Link::normalSession() const
	{
		return sc<Session&>( session() );
	}

	MM_TSC MasterSession& Link::masterSession() const
	{
		return sc<MasterSession&>( session() );
	}

	MM_TS void Link::updateCustomMatchmakingMd( const MetaData& md )
	{
		scoped_spinlock lk( m_MatchmakingDataMutex);
		m_CustomMatchmakingMd = md;
	}

	MM_TS bool Link::disconnect(bool isKick, bool sendMsg)
	{
		auto ls = get<LinkState>();
		if ( ls )
		{
			return ls->disconnect( isKick, sendMsg );
		}
		else
		{
			LOGW( "Link %s received disconnect while it had no linkState.", info() );
		}
		return false;
	}

	MM_TS void Link::createGroup( const string& typeName, const BinSerializer& initData )
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionLink>()->addNewPendingGroup( sc<Session&>(session()), varVec, typeName, initData, nullptr ); // makes copy
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
		EComponentType ct = (EComponentType) compType;

		switch ( ct )
		{
		case EComponentType::ReliableSend:
			getOrAdd<ReliableAckSend>(channel)->addAck( pi.m_Sequence );
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
			LOGW( "Unknown stream type %d detected. Packet ignored.", (u32)ct );
			break;
		}
	}

	MM_TS void Link::send(const byte* data, u32 length)
	{
		if ( !m_SockAddrPair.m_Socket )
		{
			LOGC( "Invalid socket, cannot send." );
			return;
		}

		i32 err = 0;
		ESendResult res = m_SockAddrPair.m_Socket->send( sc<const Endpoint&>( *m_SockAddrPair.m_Address ), data, length, &err );
	
	//	thread_local static u32 kt=0;	
	//	LOG( "Send ... %d", kt++ );

		if ( err != 0 && ESendResult::Error==res ) /* ignore err if socket gets closed */
		{
			LOGW( "Socket send error %d.", err );
		}
	}

	MM_TO_PTR_IMP( Link )

}
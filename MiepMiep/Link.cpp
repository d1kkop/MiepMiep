#include "Link.h"
#include "Session.h"
#include "Network.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "Endpoint.h"
#include "Socket.h"
#include "LinkState.h"
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
		ParentNetwork(network)
	{
	}

	Link::~Link()
	{
		sptr<SocketSetManager> ss = m_Network.get<SocketSetManager>();
		if ( ss )
		{
			ss->removeSocket( m_SockAddrPair.m_Socket );
		}
		LOG( "Link %s destroyed.", info() );
	}

	bool Link::operator<( const Link& o ) const
	{
		return getSocketAddrPair() < o.getSocketAddrPair();
	}

	bool Link::operator==( const Link& o ) const
	{
		return getSocketAddrPair() == o.getSocketAddrPair();
	}

	sptr<Link> Link::create( Network& network, SessionBase* session, const SocketAddrPair& sap )
	{
		sptr<Link> link = reserve_sp<Link, Network&>( MM_FL, network );

		i32 err=0;
		link->m_Source = Endpoint::createSource( *sap.m_Socket, &err );
		if ( err != 0 )
		{
			LOGW( "Local endpoint could not be resolved, error %d.", err );
			return nullptr;
		}

		link->m_SockAddrPair = sap;
		if ( session )
		{
			if ( !session->addLink( link ) )
			{
				return nullptr;
			}
		}
        else
        {
            // Only log if created without session otherwise double messages for creating and adding a new link.
		    LOG( "Created new link to %s.", link->info() );
        }
		return link;
	}

	INetwork& Link::network() const
	{
		return m_Network;
	}

	ISession& Link::session() const
	{
		return *m_Session;
	}

	bool Link::isConnected() const
	{
		auto ls = get<LinkState>();
		return ls && ls->state() == ELinkState::Connected;
	}

	SessionBase* Link::getSession() const
	{
		return m_Session.get();
	}

	const char* Link::ipAndPort() const
	{
		assert(m_SockAddrPair.m_Address);
		return m_SockAddrPair.m_Address->toIpAndPort();
	}

	const char* Link::info() const
	{
		static thread_local char buff[128];
		Platform::formatPrint(buff, sizeof(buff), "%s (src_port: %d, socket: %d)", ipAndPort(), source().port(), socket().id());
		return buff;
	}

	const SocketAddrPair& Link::getSocketAddrPair() const
	{
		return m_SockAddrPair;
	}

	void Link::setSession( SessionBase& session )
	{
		assert( !m_Session );
		m_Session = session.to_ptr();
	}

	Session& Link::normalSession() const
	{
		return sc<Session&>( session() );
	}

	MasterSession& Link::masterSession() const
	{
		return sc<MasterSession&>( session() );
	}

	void Link::updateCustomMatchmakingMd( const MetaData& md )
	{
		m_CustomMatchmakingMd = md;
	}

	bool Link::disconnect(bool isKick, bool isReceive, bool removeLink)
	{
		auto ls = get<LinkState>();
		if ( ls )
		{
			return ls->disconnect( isKick, isReceive, removeLink );
		}
		else if ( isReceive )
		{
			LOGW( "Link %s received disconnect while it had no linkState.", info() );
		}
		return false;
	}

	void Link::createGroup( const string& typeName, const BinSerializer& initData )
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionLink>()->addNewPendingGroup( sc<Session&>(session()), varVec, typeName, initData, nullptr ); // makes copy
		varVec.clear();
	}

	void Link::destroyGroup(u32 id)
	{
		// TODO
	}

	void Link::receive(BinSerializer& bs)
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
			getOrAdd<ReliableAckRecv>(channel)->receive( bs );
			break;

		case EComponentType::ReliableNewAckSend:
			getOrAdd<ReliableNewestAckRecv>()->receive( bs );
			break;

		default:
			LOGW( "Unknown stream type %d detected. Packet ignored.", (u32)ct );
			break;
		}
	}

	void Link::send(const byte* data, u32 length)
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
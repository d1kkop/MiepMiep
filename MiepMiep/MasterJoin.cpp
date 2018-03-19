#include "MasterJoin.h"
#include "MiepMiep.h"
#include "Network.h"
#include "Link.h"
#include "Platform.h"
#include "Socket.h"
#include "MasterServer.h"
#include "NetworkListeners.h"
#include "LinkState.h"


namespace MiepMiep
{
	// ---------- Event -------------------------------------------------------------------------------

	struct EventRegisterResult : EventBase
	{
		EventRegisterResult( const Link& link, const sptr<MasterJoin>& mj, bool succes ):
			EventBase( link ),
			m_Mj( mj ),
			m_Succes( succes )
		{
		}

		void process() override
		{
			if ( m_Mj ) m_Mj->getRegisterCb()(*m_Link, m_Succes);
		}

		sptr<const MasterJoin> m_Mj;
		bool m_Succes;
	};

	struct EventJoinResult : EventBase
	{
		EventJoinResult( const Link& link, const sptr<MasterJoin>& mj, EJoinServerResult joinRes  ):
			EventBase( link ),
			m_Mj( mj ),
			m_JoinRes( joinRes )
		{
		}

		void process() override
		{
			if ( m_Mj ) m_Mj->getJoinCb()(*m_Link, m_JoinRes);
		}

		sptr<const MasterJoin> m_Mj;
		EJoinServerResult m_JoinRes;
	};


	// ---------- RPC -------------------------------------------------------------------------------

	// [bool : succes or fail]
	MM_RPC(masterRpcRegisterResult, bool)
	{
		RPC_BEGIN();
		l.pushEvent<EventRegisterResult>( l.get<MasterJoin>(), get<0>( tp ) );
	}

	// [masterServerId, serverName, gameType, initialRating]
	MM_RPC(masterRpcRegisterServer, string, string, string, float, MetaData)
	{
		RPC_BEGIN();
		const string& name = get<0>( tp );
		const string& pw   = get<1>( tp );
		const string& type = get<2>( tp );
		float rating       = get<3>( tp );
		const MetaData& md = get<4>( tp );
		if ( nw.getOrAdd<MasterServer>()->registerServer( l.socket(), l.destination(), name, pw, type, rating, md ) )
		{
			l.callRpc<masterRpcRegisterResult, bool>(true, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master register server request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterRpcRegisterResult, bool>(false, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master register server request from %s failed.", l.info());
		}
	}

	// [bool : succes or fail, u32 linkId, addr to server or client]
	MM_RPC(masterRpcJoinResult, bool, u32, sptr<IAddress>)
	{
		RPC_BEGIN();
		bool bSucces = get<0>( tp );
		u32 linkId   = get<1>(tp);
		sptr<MasterJoin> mj = l.get<MasterJoin>(); /* link is on which the rpc was received */
		if ( bSucces )
		{
			// Forward ConnectResult to a single joinMaster->connect result
			auto lamConnResult = [=, sl = l.to_ptr()] ( EConnectResult res ) mutable
			{
				switch ( res )
				{
				case EConnectResult::Fine:
					sl->pushEvent<EventJoinResult>( mj, EJoinServerResult::Fine );
					break;
				case EConnectResult::Timedout:
					sl->pushEvent<EventJoinResult>( mj, EJoinServerResult::TimedOut );
					break;
				case EConnectResult::InvalidPassword:
					sl->pushEvent<EventJoinResult>( mj, EJoinServerResult::InvalidPassword );
					break;
				case EConnectResult::MaxConnectionsReached:
					sl->pushEvent<EventJoinResult>( mj, EJoinServerResult::MaxConnectionsReached );
					break;
				case EConnectResult::AlreadyConnected:
					sl->pushEvent<EventJoinResult>( mj, EJoinServerResult::AlreadyConnected );
					break;
				default:
					LOGC( "Invalid event connect result detected." );
					sl->pushEvent<EventJoinResult>( mj, EJoinServerResult::LogicError );
					break;
				}
			};
			
			// Try connect to received addr. If connection was already accepted, the connect member returns 'true' immediately.
			const sptr<IAddress>& to = get<2>( tp );
			sptr<Link> newLink = nw.getOrAdd<LinkManager>()->add( *to, linkId );
			if ( !(newLink && mj && newLink->getOrAdd<LinkState>()->connect( mj->password(), mj->metaData(), lamConnResult )) )
			{
				l.pushEvent<EventJoinResult>( mj, EJoinServerResult::LogicError );
			}
			// else -> event will arise from connect request (or timeout event, if connect takes too long..).
		}
		else
		{
			l.pushEvent<EventJoinResult>( mj, EJoinServerResult::NoMatchesFound );
		}
	}

	// [serverName, gameType, minRating, maxRating, minPlayers, maxPlayers]
	MM_RPC(masterRpcJoinServer, string, string, string, float, float, u32, u32)
	{
		RPC_BEGIN();
		const string& name = get<0>(tp);
		const string& pw   = get<1>(tp);
		const string& type = get<2>(tp);
		float minRating = get<3>(tp);
		float maxRating = get<4>(tp);
		u32 minPlayers  = get<5>(tp);
		u32 maxPlayers  = get<6>(tp);
		SocketAddrPair serverSap = nw.getOrAdd<MasterServer>()->findServerFromFilter( name, pw, type, minRating, maxRating, minPlayers, maxPlayers );
		if ( serverSap.m_Address )
		{
			// Make unique link id, which the server and client both use to connect (handshake).
			u32 linkId = rand();
			// Send connect request from both server->client and client->server.
			l.callRpc<masterRpcJoinResult, bool, u32, sptr<IAddress>>(true, linkId, serverSap.m_Address->getCopy(), false, false, MM_RPC_CHANNEL, nullptr);
			sptr<Link> servLink = nw.getOrAdd<LinkManager>()->get( serverSap );
			if ( servLink )
			{
				servLink->callRpc<masterRpcJoinResult, bool, u32, sptr<IAddress>>(true, linkId, l.destination().getCopy(), false, false, MM_RPC_CHANNEL, nullptr);
			}
			LOG("New master join request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterRpcJoinResult, bool, u32, sptr<IAddress>>(false, -1, sptr<IAddress>(), false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master join server request from %s failed.", l.info());
		}
	}



	// ---------- MasterJoinData -------------------------------------------------------------------------------

	MasterJoin::MasterJoin(Link& link, const string& name, const string& pw, const string& type, float rating, const MetaData& md):
		ParentLink(link),
		m_Name(name),
		m_Pw(pw),
		m_Type(type),
		m_Rating(rating),
		m_Md(md)
	{
	}

	MasterJoin::~MasterJoin()
	{
	}

	MM_TS void MasterJoin::registerServer(const MetaData& customFilterMd, const function<void (const ILink& link, bool)>& cb )
	{
		m_RegisterCb = cb;
		m_Link.callRpc<masterRpcRegisterServer, string, string, string, float, MetaData>(
			/* data */	m_Name, m_Pw, m_Type, m_Rating, customFilterMd,
			/* rpc */	false, false, MM_RPC_CHANNEL, nullptr);
	}

	MM_TS void MasterJoin::joinServer(u32 minPlayers, u32 maxPlayers, float minRating, float maxRating, const function<void (const ILink& link, EJoinServerResult)>& cb )
	{
		m_JoinCb = cb;
		m_Link.callRpc<masterRpcJoinServer, string, string, string, float, float, u32, u32>(
			/* data */		m_Name, m_Pw, m_Type, minRating, maxRating, minPlayers, maxPlayers,
			/* rpc */		false, false, MM_RPC_CHANNEL, nullptr);
	}
}
#include "MasterLinkData.h"
#include "MiepMiep.h"
#include "Network.h"
#include "Link.h"
#include "Platform.h"
#include "Socket.h"
#include "MasterServer.h"
#include "NetworkListeners.h"
#include "LinkState.h"
#include "MasterServer.h"


namespace MiepMiep
{
	// ---------- Event -------------------------------------------------------------------------------

	struct EventRegisterResult : EventBase
	{
		EventRegisterResult( const Link& link, const sptr<MasterLinkData>& mj, bool succes ):
			EventBase( link ),
			m_Mj( mj ),
			m_Succes( succes )
		{
		}

		void process() override
		{
			if ( m_Mj ) m_Mj->getRegisterCb()(*m_Link, m_Succes);
		}

		sptr<const MasterLinkData> m_Mj;
		bool m_Succes;
	};

	struct EventJoinResult : EventBase
	{
		EventJoinResult( const Link& link, const sptr<MasterLinkData>& mj, EJoinServerResult joinRes  ):
			EventBase( link ),
			m_Mj( mj ),
			m_JoinRes( joinRes )
		{
		}

		void process() override
		{
			if ( m_Mj ) m_Mj->getJoinCb()(*m_Link, m_JoinRes);
		}

		sptr<const MasterLinkData> m_Mj;
		EJoinServerResult m_JoinRes;
	};


	// ---------- RPC -------------------------------------------------------------------------------

	// [bool : succes or fail]
	MM_RPC(masterRpcRegisterResult, bool)
	{
		RPC_BEGIN();
		bool succes = get<0>(tp);
		l.pushEvent<EventRegisterResult>( l.get<MasterLinkData>(), succes );
	}

	MM_RPC(masterRpcRegisterServer, MasterSessionData)
	{
		RPC_BEGIN();
		const MasterSessionData& data = get<0>(tp);
		if ( nw.getOrAdd<MasterServer>()->registerServer( l.to_ptr(), data ) )
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
		u32 linkId   = get<1>( tp );
		sptr<MasterLinkData> mj = l.get<MasterLinkData>(); /* link is on which the rpc was received */
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
			//const sptr<IAddress>& to = get<2>( tp );
			//sptr<Link> newLink = nw.getOrAdd<LinkManager>()->add( SocketAddrPair( l.socket(), *to ), linkId );
			//if ( !(newLink && mj && newLink->getOrAdd<LinkState>()->connect( mj->password(), mj->metaData(), lamConnResult )) )
			//{
			//	l.pushEvent<EventJoinResult>( mj, EJoinServerResult::LogicError );
			//}
			// else -> event will arise from connect request (or timeout event, if connect takes too long..).
		}
		else
		{
			l.pushEvent<EventJoinResult>( mj, EJoinServerResult::NoMatchesFound );
		}
	}

	MM_RPC(masterRpcJoinServer, SearchFilter)
	{
		RPC_BEGIN();
		const SearchFilter& sf = get<0>(tp);
		SocketAddrPair serverSap = nw.getOrAdd<MasterServer>()->findServerFromFilter( sf );
		if ( serverSap )
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

	MasterLinkData::MasterLinkData(Link& link):
		ParentLink(link)
	{
	}

	MasterLinkData::~MasterLinkData()
	{
	}

	MM_TS void MasterLinkData::registerServer( const function<void( const ILink& link, bool )>& cb, const MasterSessionData& data )
	{
		m_RegisterCb = cb;
		m_Link.callRpc<masterRpcRegisterServer, MasterSessionData>( data, false, false, MM_RPC_CHANNEL, nullptr );
	}

	MM_TS void MasterLinkData::joinServer( const function<void( const ILink& link, EJoinServerResult )>& cb, const SearchFilter& sf )
	{
		m_JoinCb = cb;
		m_Link.callRpc<masterRpcJoinServer, SearchFilter>( sf, false, false, MM_RPC_CHANNEL, nullptr );
	}

}
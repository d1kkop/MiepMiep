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
			EventBase( link, false ),
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
			EventBase( link, false ),
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

	// Executes on client.
	// [bool : succes or fail]
	MM_RPC(masterLinkRpcRegisterResult, bool)
	{
		RPC_BEGIN();
		bool succes = get<0>(tp);
		sptr<MasterLinkData> mj = l.get<MasterLinkData>();
		assert(mj);
		l.pushEvent<EventRegisterResult>( mj, succes );
	}

	// Executes on master server!
	MM_RPC(masterLinkRpcRegisterServer, MasterSessionData, MetaData)
	{
		RPC_BEGIN_NO_S();
		const MasterSessionData& data = get<0>(tp);
		if ( nw.getOrAdd<MasterServer>()->registerSession( l.to_ptr(), data, get<1>(tp ) ) )
		{
			l.callRpc<masterLinkRpcRegisterResult, bool>(true, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master register server request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterLinkRpcRegisterResult, bool>(false, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master register server request from %s failed.", l.info());
		}
	}

	// Executes on client.
	// [bool : succes or fail]
	MM_RPC(masterLinkRpcJoinResult, bool)
	{
		RPC_BEGIN();
		bool bSucces = get<0>( tp );
		sptr<MasterLinkData> mj = l.get<MasterLinkData>(); 
		assert(mj);
		l.pushEvent<EventJoinResult>( mj, bSucces ? EJoinServerResult::Fine : EJoinServerResult::NoMatchesFound );
	}

	// Executes on master server!
	MM_RPC(masterLinkRpcJoinServer, SearchFilter, MetaData)
	{
		RPC_BEGIN_NO_S();
		const SearchFilter& sf = get<0>(tp);
		const MetaData& joinMatchmakingMd = get<1>(tp);
		auto* mSession = nw.getOrAdd<MasterServer>()->findServerFromFilter( sf );
		if ( mSession && mSession->tryJoin( l, joinMatchmakingMd ) )
		{
			l.callRpc<masterLinkRpcJoinResult, bool>( true, false, false, MM_RPC_CHANNEL, nullptr );
			LOG("New master join request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterLinkRpcJoinResult, bool>( false, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master join server request from %s failed.", l.info());
		}
	}


	MM_RPC( masterLinkRpcStartSession )
	{
		RPC_BEGIN();
		MasterSession* mses = l.masterSession();
		if ( mses )
		{
			if ( !mses->start() )
			{
				LOGW( "Tried to start an already started master session." );
			}
		}
		else
		{
			LOGW( "Link %s tried to start a master session while it had no such session associated with it.", l.info() );
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

	MM_TS void MasterLinkData::registerServer( const function<void( const ILink& link, bool )>& cb, const MasterSessionData& data,
											   const MetaData& customMatchmakingMd )
	{
		// Cb lock
		{
			scoped_lock lk( m_DataMutex );
			m_RegisterCb = cb;
		}
		m_Link.callRpc<masterLinkRpcRegisterServer, MasterSessionData, MetaData>( data, customMatchmakingMd, false, false, MM_RPC_CHANNEL, nullptr );
	}

	MM_TS void MasterLinkData::joinServer( const function<void( const ILink& link, EJoinServerResult )>& cb, const SearchFilter& sf,
										   const MetaData& customMatchmakingMd )
	{
		// Cb lock
		{
			scoped_lock lk( m_DataMutex );
			m_JoinCb = cb;
		}
		m_Link.callRpc<masterLinkRpcJoinServer, SearchFilter, MetaData>( sf, customMatchmakingMd, false, false, MM_RPC_CHANNEL, nullptr );
	}

}
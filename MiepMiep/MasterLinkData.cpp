#include "MasterLinkData.h"
#include "NetworkEvents.h"
#include "MasterServer.h"
#include "MasterSession.h"


namespace MiepMiep
{
	// ---------- Event -------------------------------------------------------------------------------

	struct EventRegisterResult : IEvent
	{
		EventRegisterResult( const sptr<Link>& link, const sptr<MasterLinkData>& mj, bool succes ):
			IEvent( link, false ),
			m_Mj( mj ),
			m_Succes( succes )
		{
		}

		void process() override
		{
			m_Mj->getRegisterCb()(m_Link->session(), m_Succes);
		}

		sptr<const MasterLinkData> m_Mj;
		bool m_Succes;
	};

	struct EventJoinResult : IEvent
	{
		EventJoinResult( const sptr<Link>& link, const sptr<MasterLinkData>& mj, bool joinRes  ):
			IEvent( link, false ),
			m_Mj( mj ),
			m_JoinRes( joinRes )
		{
		}

		void process() override
		{
			m_Mj->getJoinCb()(m_Link->session(), m_JoinRes);
		}

		sptr<const MasterLinkData> m_Mj;
		bool m_JoinRes;
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
        if ( mj )
        {
		    l.pushEvent<EventRegisterResult>( mj, succes );
        }
        else 
        {
            LOGW("Received master link register result while there was no masterLinkData object on the link.");
        }
	}

	// Executes on master server!
	MM_RPC(masterLinkRpcRegisterServer, MasterSessionData, MetaData)
	{
		RPC_BEGIN_NO_S();
		const MasterSessionData& data = get<0>(tp);
		const MetaData& registerMd = get<1>(tp);
		l.updateCustomMatchmakingMd( registerMd );
		sptr<MasterSession> mSession = nw.getOrAdd<MasterServer>()->registerSession( l.to_ptr(), data );
		if ( mSession )
		{
			l.setSession( *mSession );
			mSession->addLink( l.to_ptr() );
			l.callRpc<masterLinkRpcRegisterResult, bool>(true, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master session with name %s.", mSession->name());
		}
		else
		{
			l.callRpc<masterLinkRpcRegisterResult, bool>(false, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("Failed to create new master session with name %s.", mSession->name());
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
        if ( mj )
        {
		    l.pushEvent<EventJoinResult>( mj, bSucces );
        }
        else
        {
            LOGW("Received masterLinkRpcJoinResult while there was no masterLinkData object on the link.");
        }
	}

	// Executes on master server!
	MM_RPC(masterLinkRpcJoinServer, SearchFilter, MetaData)
	{
		RPC_BEGIN_NO_S();
		const SearchFilter& sf = get<0>(tp);
		const MetaData& joinMatchmakingMd = get<1>(tp);
		l.updateCustomMatchmakingMd( joinMatchmakingMd ); // TODO change for var group perhaps
		sptr<MasterSession> mSession = nw.getOrAdd<MasterServer>()->findServerFromFilter( sf );
		if ( mSession )
		{
			l.setSession( *mSession );
			mSession->addLink( l.to_ptr() );
			mSession->sendConnectRequests( l );
			l.callRpc<masterLinkRpcJoinResult, bool>( true, false, false, MM_RPC_CHANNEL, nullptr );
			LOG("New master join request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterLinkRpcJoinResult, bool>( false, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master join server request from %s failed.", l.info());
		}
	}

	// Executed on master server.
	MM_RPC( masterLinkRpcStartSession )
	{
		RPC_BEGIN();
		MasterSession& mses = l.masterSession();
		if ( !mses.start() )
		{
			LOGW( "Tried to start an already started master session." );
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

	void MasterLinkData::registerServer( const function<void( ISession&, bool )>& cb, const MasterSessionData& data,
										 const MetaData& customMatchmakingMd )
	{
        assert(!m_RegisterCb); // Should only be set once.
		m_RegisterCb = cb;
		m_Link.callRpc<masterLinkRpcRegisterServer, MasterSessionData, MetaData>( data, customMatchmakingMd, false, false, MM_RPC_CHANNEL, nullptr );
	}

	void MasterLinkData::joinServer( const function<void( ISession&, bool )>& cb, const SearchFilter& sf,
									 const MetaData& customMatchmakingMd )
	{
        assert(!m_JoinCb); // Should only be set once.
		m_JoinCb = cb;
		m_Link.callRpc<masterLinkRpcJoinServer, SearchFilter, MetaData>( sf, customMatchmakingMd, false, false, MM_RPC_CHANNEL, nullptr );
	}

}
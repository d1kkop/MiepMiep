#include "MasterLink.h"
#include "NetworkEvents.h"
#include "MasterServer.h"
#include "MasterSession.h"


namespace MiepMiep
{
	// ---------- Event -------------------------------------------------------------------------------

	struct EventRegisterResult : IEvent
	{
		EventRegisterResult( const sptr<Link>& link, const sptr<MasterLink>& mj, bool succes ):
			IEvent( link, false ),
			m_Mj( mj ),
			m_Succes( succes )
		{
		}

		void process() override
		{
			m_Mj->getRegisterCb()(m_Link->session(), m_Succes);
		}

		sptr<const MasterLink> m_Mj;
		bool m_Succes;
	};

	struct EventJoinResult : IEvent
	{
		EventJoinResult( const sptr<Link>& link, const sptr<MasterLink>& mj, bool joinRes  ):
			IEvent( link, false ),
			m_Mj( mj ),
			m_JoinRes( joinRes )
		{
		}

		void process() override
		{
			m_Mj->getJoinCb()(m_Link->session(), m_JoinRes);
		}

		sptr<const MasterLink> m_Mj;
		bool m_JoinRes;
	};


	// ---------- RPC -------------------------------------------------------------------------------

	// Executes on client.
	// [bool : succes or fail]
	MM_RPC(masterLinkRpcRegisterResult, bool)
	{
		RPC_BEGIN();
        assert(s.matchMaker()==link->to_ptr());
		bool succes = get<0>(tp);
		sptr<MasterLink> mj = l.get<MasterLink>();
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
        // L.ptr() (link) becomes host of this master session immediately. A master session always has a host.
		sptr<MasterSession> mSession = nw.getOrAdd<MasterServer>()->registerSession( l.to_ptr(), data );
		if ( mSession )
		{
			mSession->addLink( l.to_ptr() );
			l.callRpc<masterLinkRpcRegisterResult, bool>(Is_Succes, No_Local, No_Relay, MM_RPC_CHANNEL, nullptr);
			LOG("New master session with name %s.", mSession->name());
		}
		else
		{
			l.callRpc<masterLinkRpcRegisterResult, bool>(Is_Fail, No_Local, No_Relay, MM_RPC_CHANNEL, nullptr);
			LOG("Failed to create new master session with name %s.", mSession->name());
		}
	}

	// Executes on client.
	// [bool : succes or fail]
	MM_RPC(masterLinkRpcJoinResult, bool)
	{
		RPC_BEGIN();
        assert(s.matchMaker()==link->to_ptr());
		bool bSucces = get<0>( tp );
		sptr<MasterLink> mj = l.get<MasterLink>(); 
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
			mSession->addLink( l.to_ptr() );
			mSession->sendConnectRequests( l );
			l.callRpc<masterLinkRpcJoinResult, bool>( Is_Succes, No_Local, No_Relay, MM_RPC_CHANNEL, nullptr );
			LOG("New master join request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterLinkRpcJoinResult, bool>( Is_Fail, No_Local, No_Relay, MM_RPC_CHANNEL, nullptr);
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

	MasterLink::MasterLink(Link& link):
		ParentLink(link)
	{
	}

	MasterLink::~MasterLink()
	{
	}

	bool MasterLink::registerServer( const function<void( ISession&, bool )>& cb, const MasterSessionData& data,
										 const MetaData& customMatchmakingMd )
	{
        if (m_RegisterCb)
        {
            LOGC("Can only call registerServer once.");
            return false;
        }
		m_RegisterCb = cb;
		m_Link.callRpc<masterLinkRpcRegisterServer, MasterSessionData, MetaData>( data, customMatchmakingMd, false, false, MM_RPC_CHANNEL, nullptr );
        return true;
	}

	bool MasterLink::joinServer( const function<void( ISession&, bool )>& cb, const SearchFilter& sf,
									 const MetaData& customMatchmakingMd )
	{
        if (m_JoinCb)
        {
            LOGC("Can only call joinServer once.");
            return false;
        }
		m_JoinCb = cb;
		m_Link.callRpc<masterLinkRpcJoinServer, SearchFilter, MetaData>( sf, customMatchmakingMd, false, false, MM_RPC_CHANNEL, nullptr );
        return true;
	}

}
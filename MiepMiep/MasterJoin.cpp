#include "MasterJoin.h"
#include "MiepMiep.h"
#include "Network.h"
#include "Link.h"
#include "Platform.h"
#include "Socket.h"
#include "MasterServer.h"


namespace MiepMiep
{
	// ---------- RPC -------------------------------------------------------------------------------

	// [bool : succes or fail]
	MM_RPC(masterJoinRegisterResult, bool)
	{
		RPC_BEGIN();
	}

	// [masterServerId, serverName, gameType, initialRating]
	MM_RPC(masterjoinRegister, u32, string, string, string, float, MetaData)
	{
		RPC_BEGIN();
		const string& name = get<1>(tp);
		const string& pw   = get<2>(tp);
		const string& type = get<3>(tp);
		float rating = get<4>(tp);
		const MetaData& md = get<5>(tp);
		if ( nw.getOrAdd<MasterServer>()->registerServer( l.socket(), l.destination(), name, pw, type, rating, md ) )
		{
			l.callRpc<masterJoinRegisterResult, bool>(true, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master register server request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterJoinRegisterResult, bool>(false, false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master register server request from %s failed.", l.info());
		}
	}


	// [uniqueLinkId, bool : succes or fail, addr to server or client]
	MM_RPC(masterJoinJoinResult, u32, bool, sptr<IAddress>)
	{
		RPC_BEGIN();
		u32 linkId = get<0>(tp);
		bool bSucces = get<1>(tp);
		if ( bSucces )
		{
			const sptr<IAddress>& to = get<2>(tp);
			sptr<Link> link = nw.getOrAdd<LinkManager>()->add( linkId, to );
			if ( link )
			{
				//link->getOrAdd<LinkState>()->connect( "", 
			}
		}
	}

	// [masterServerId, serverName, gameType, minRating, maxRating, minPlayers, maxPlayers]
	MM_RPC(masterJoinJoin, u32, string, string, string, float, float, u32, u32)
	{
		RPC_BEGIN();
		const string& name = get<1>(tp);
		const string& pw   = get<2>(tp);
		const string& type = get<3>(tp);
		float minRating = get<4>(tp);
		float maxRating = get<5>(tp);
		u32 minPlayers  = get<6>(tp);
		u32 maxPlayers  = get<7>(tp);
		SocketAddrPair serverSap = nw.getOrAdd<MasterServer>()->findServerFromFilter( name, pw, type, minRating, maxRating, minPlayers, maxPlayers );
		if ( serverSap.m_Address )
		{
			// Make unique link id, which the server and client both use to connect (handshake).
			u32 linkId = rand();
			// Send connect request from both server to client and client to server.
			l.callRpc<masterJoinJoinResult, bool, u32, sptr<IAddress>>(true, linkId, serverSap.m_Address->getCopy(), false, false, MM_RPC_CHANNEL, nullptr);
			sptr<Link> servLink = nw.getOrAdd<LinkManager>()->get( serverSap );
			if ( servLink )
			{
				servLink->callRpc<masterJoinJoinResult, bool, u32, sptr<IAddress>>(true, linkId, l.destination().getCopy(), false, false, MM_RPC_CHANNEL, nullptr);
			}
			LOG("New master join request from %s succesful.", l.info());
		}
		else
		{
			l.callRpc<masterJoinJoinResult, bool, sptr<IAddress>>(false, sptr<IAddress>(), false, false, MM_RPC_CHANNEL, nullptr);
			LOG("New master join server request from %s failed.", l.info());
		}
	}



	// ---------- MasterJoinData -------------------------------------------------------------------------------

	MasterJoin::MasterJoin(Link& link, const string& name, const string& pw, const string& type, float rating):
		ParentLink(link),
		m_Name(name),
		m_Pw(pw),
		m_Type(type),
		m_Rating(rating)
	{
	}

	MasterJoin::~MasterJoin()
	{
	}

	MM_TS void MasterJoin::registerServer(const MetaData& customFilterMd)
	{
		m_Link.callRpc<masterjoinRegister, u32, string, string, string, float, MetaData>(
			/* data */	0, m_Name, m_Pw, m_Type, m_Rating, customFilterMd,
			/* rpc */	false, false, MM_RPC_CHANNEL, nullptr);
	}

	MM_TS void MasterJoin::joinServer(u32 minPlayers, u32 maxPlayers, float minRating, float maxRating)
	{
		m_Link.callRpc<masterJoinJoin, u32, string, string, string, float, float, u32, u32>(
			/* data */		0, m_Name, m_Pw, m_Type, minRating, maxRating, minPlayers, maxPlayers,
			/* rpc */		false, false, MM_RPC_CHANNEL, nullptr);
	}
}
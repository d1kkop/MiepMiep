#include "MasterJoinData.h"
#include "MiepMiep.h"
#include "Network.h"
#include "Link.h"
#include "Platform.h"
#include "Socket.h"


namespace MiepMiep
{
	// ---------- RPC -------------------------------------------------------------------------------

	// [masterServerId, serverName, gameType, initialRating]
	MM_RPC(masterJoinDataRegisterServer, u32, string, string, float)
	{
		RPC_BEGIN();
	}

	// [masterServerId, serverName, gameType, initalRating, minRating, maxRating, minPlayers, maxPlayers]
	MM_RPC(masterJoinDataJoinServer, u32, string, string, float, float, float, u32, u32)
	{
		RPC_BEGIN();
	}

	// ---------- MasterJoinData -------------------------------------------------------------------------------

	MasterJoinData::MasterJoinData(Link& link, const string& name, const string& pw, const string& type, float rating):
		ParentLink(link),
		m_Name(name),
		m_Pw(pw),
		m_Type(type),
		m_Rating(rating),
		m_IsRegister(false)
	{

		network().addConnectionListener( this );
	}

	MasterJoinData::~MasterJoinData()
	{
		network().removeConnectionListener( this );
	}

	MM_TS void MasterJoinData::setServerProps(const MetaData& hostMd)
	{
		scoped_spinlock lk( m_DataMutex );
		m_MetaData = hostMd;
		m_IsRegister = true;
	}

	MM_TS void MasterJoinData::setJoinFilter(const MetaData& joinMd, u32 minPlayers, u32 maxPlayers, float minRating, float maxRating)
	{
		scoped_spinlock lk( m_DataMutex );
		m_MetaData	 = joinMd;
		m_MinRating  = minRating;
		m_MaxRating  = maxRating;
		m_MinPlayers = minPlayers;
		m_MaxPlayers = maxPlayers;
		m_IsRegister = false;
	}


	MM_TS void MasterJoinData::onConnectResult(const ILink& link, EConnectResult res)
	{
		assert( &link == &m_Link );

		switch (res)
		{
		case EConnectResult::Fine:
			{
				u32 msId = 0; // TODO set to masterServerId
				if ( m_IsRegister )
				{
					toLink(link).callRpc<masterJoinDataRegisterServer, u32, string, string, float>(
				/* data */	msId, m_Name, m_Type, m_Rating, 
				/* rpc */	false, false, MM_RPC_CHANNEL, nullptr );
				}
				else
				{
					toLink(link).callRpc<masterJoinDataJoinServer, u32, string, string, float, float, float, u32, u32>(
				/* data */		msId, m_Name, m_Type, m_Rating, m_MinRating, m_MaxRating, m_MinPlayers, m_MaxPlayers,
				/* rpc */		false, false, MM_RPC_CHANNEL, nullptr );
				}
			}
			LOG("Connection to master server %s established.", m_Link.info());
			break;
		case EConnectResult::Timedout:
			LOG("Connection to master server %s timed out.", m_Link.info());
			break;
		case EConnectResult::InvalidPassword:
			LOG("Connection to master server %s failed. Invalid password.", m_Link.info());
			break;
		case EConnectResult::MaxConnectionsReached:
			LOG("Connection to master server %s failed. Max connections reached.", m_Link.info());
			break;
		case EConnectResult::AlreadyConnected:
			LOG("Connection to master server %s failed. Already registered.", m_Link.info());
			break;
		default:
			LOG("Connection to master server %s failed. Unknown result returned.", m_Link.info());
			break;
		}
	}

	MM_TS void MasterJoinData::onNewConnection(const ILink& link, const IAddress& remote)
	{

	}

	MM_TS void MasterJoinData::onDisconnect(const ILink& link, EDisconnectReason reason, const IAddress& remote)
	{
		assert( &link == &m_Link );

		switch (reason)
		{
		case EDisconnectReason::Closed:
			LOG("Connection to master server %s closed.", m_Link.info());
			break;
		case EDisconnectReason::Kicked:
			LOG("Connection to master server %s droped. We were kicked.", m_Link.info());
			break;
		case EDisconnectReason::Lost:
			LOG("Connection to master server %s timed out.", m_Link.info());
			break;
		}
	}
}
#pragma once

#include "Component.h"
#include "Memory.h"
#include "Link.h"
#include "Variables.h"


namespace MiepMiep
{
	enum class EConnectState
	{
		Unknown,
		Connecting,
		Connected,
		Disconnected
	};

	enum class EDisconnectReason
	{
		Kicked,
		Closed,
		Lost
	};

	enum class EConnectFailReason
	{
		AlreadyConnected,
		InvalidPassword,
		MaxClientsReached
	};


	class LinkState: public Parent<Link>, public IComponent, public ITraceable
	{
	public:
		LinkState(Link& link);
		static EComponentType compType() { return EComponentType::LinkState; }
		
		static void createFromRemoteCall(INetwork& network, const IEndpoint& etp);
		MM_NETWORK_CREATABLE()
		{
			createFromRemoteCall( network, etp );
		}

		bool connect();

		NetUint32 m_ConnectState;

		EConnectState m_State;
		EDisconnectReason m_DiscReason;
		EConnectFailReason m_ConnFailReason;
	};
}

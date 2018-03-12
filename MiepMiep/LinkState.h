#pragma once

#include "Component.h"
#include "Memory.h"
#include "Link.h"


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


	class LinkState: public ParentLink, public IComponent, public ITraceable
	{
	public:
		LinkState(Link& link);
		static EComponentType compType() { return EComponentType::LinkState; }

		/*	Returns only true if current state is 'unknown'.
			When state is 'unknown', state is changed to 'conneecting' and true is returend. */
		MM_TS bool connect(const string& pw, const MetaData& md=MetaData());

		/*	Returns only true if current state is 'unknown.' 
			Only when the state is 'unknown', the connection becomes 'connected'. */
		MM_TS bool acceptConnect();


		EConnectState m_State;
		EDisconnectReason m_DiscReason;
		EConnectFailReason m_ConnFailReason;
		SpinLock m_StateMutex;
	};
}

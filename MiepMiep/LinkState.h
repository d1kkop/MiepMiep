#pragma once

#include "ParentLink.h"
#include "Threading.h"
#include "Component.h"
#include "Memory.h"


namespace MiepMiep
{
	enum class ELinkState
	{
		Unknown,
		Connecting,
		Connected,
		Disconnected
	};


	class LinkState: public ParentLink, public IComponent, public ITraceable
	{
	public:
		LinkState(Link& link);
		static EComponentType compType() { return EComponentType::LinkState; }

		MM_TS bool connect(const MetaData& md);
		MM_TS bool accept();
		MM_TS bool canReceiveConnect();
		MM_TS bool disconnect(bool isKick, bool isReceive, bool removeLink);
		MM_TS ELinkState state() const;

		mutable SpinLock m_StateMutex;
		ELinkState m_State;
		EDisconnectReason m_DiscReason;
		bool m_WasAccepted;
		bool m_RemoteConnectReceived;
	};
}

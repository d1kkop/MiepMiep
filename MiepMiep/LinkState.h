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

		/*	Returns only true if current state is 'unknown'.
			When state is 'unknown', state is changed to 'conneecting' and true is returend. */
		MM_TS bool connect(const string& pw, const MetaData& md, const function<void (EConnectResult)>& connectCb);

		/*	Returns only true if current state is 'unknown.' 
			Only when the state is 'unknown', the connection becomes 'connected'. */
		MM_TS bool acceptConnect();

		MM_TS ELinkState state() const;
		
		// This function is not thread safe but only set once in connect and will only be acquired upon reply. So it is thread safe to call this.
		const function<void( EConnectResult )>& getConnectCb() const { return m_ConnectCb; }

		mutable SpinLock m_StateMutex;
		ELinkState m_State;
		EDisconnectReason m_DiscReason;
		bool m_WasAccepted;
		function<void( EConnectResult )> m_ConnectCb;
	};
}

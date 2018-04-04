#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;

	class ReliableAckSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableAckSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableAckSend; }

		MM_TS void addAck( u32 ack );
		MM_TS void resend();

		// Resend only if 'a' interval has passed.
		MM_TS void intervalDispatch( u64 time );

	private:
		mutex m_PacketsMutex;
		u64 m_LastResendTS;
		vector<u32> m_ReceivedPackets;
	};
}

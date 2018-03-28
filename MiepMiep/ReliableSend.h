#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"
#include <atomic>


namespace MiepMiep
{
	class Link;
	struct NormalSendPacket;


	class ReliableSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableSend; }

		MM_TS void enqueue( const vector<sptr<const NormalSendPacket>>& rsp, class IDeliveryTrace* trace );
		MM_TS void resend();

		MM_TS void resendIfLatencyTimePassed( u64 time );
		MM_TS void dispatchAckQueueIfAggregateTimePassed( u64 time );

	private:
		mutex m_SendQueueMutex;
		u32 m_SendSequence;
		atomic<u64> m_LastResendTS;
		map<u32, sptr<const NormalSendPacket>> m_SendQueue;
	};
}

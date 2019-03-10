#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;

	/* 
		When a reliable packet is received an ack is stored for that packet in a list.
		Each interval dispatch, this list of acks is transmitted and the list is cleared.
		The idea is to aggregate small amount of ack packets in a single bigger packet.
		Replying each reliable packet with a single ack packet would result in many very small packets.
	*/
	class ReliableAckSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableAckSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableAckSend; }

		// Note: These functions must be thread safe as the ReceiveThread adds acks while the SendThread resends the ack list.
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

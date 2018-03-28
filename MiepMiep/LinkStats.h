#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentLink.h"
#include <atomic>


namespace MiepMiep
{
	class Link;


	class LinkStats: public ParentLink, public IComponent, public ITraceable
	{
	public:
		LinkStats(Link& link);
		static EComponentType compType() { return EComponentType::LinkStats; }

		// TODO
		MM_TS u32 latency() const		{ return m_Latency; }
		MM_TS u32 mtu()  const			{ return m_Mtu; }
		MM_TS u32 hostScore() const		{ return m_HostScore; }

		float resendLatencyMultiplier() const		{ return m_ResendLatencyMultiplier; }
		u32 reliableResendDelay() const				{ return u32(latency() * resendLatencyMultiplier()); }

	private:
		atomic<u32> m_Latency;
		atomic<u32> m_Mtu;
		atomic<u32> m_HostScore;
		float m_ResendLatencyMultiplier;
	};
}

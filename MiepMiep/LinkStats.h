#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;


	class LinkStats: public ParentLink, public IComponent, public ITraceable
	{
	public:
		LinkStats(Link& link);
		static EComponentType compType() { return EComponentType::LinkStats; }

		// TODO
		u32 latency() const		{ return m_Latency; }
		u32 mtu()  const		{ return m_Mtu; }

		float resendLatencyMultiplier() const		{ return m_ResendLatencyMultiplier; }
		u32 reliableResendDelay() const				{ return u32(latency() * resendLatencyMultiplier()); }

	private:
		u32 m_Latency;
		u32 m_Mtu;
		float m_ResendLatencyMultiplier;
	};
}

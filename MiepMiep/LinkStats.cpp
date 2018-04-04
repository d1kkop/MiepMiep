#include "LinkStats.h"


namespace MiepMiep
{
	LinkStats::LinkStats(Link& link):
		ParentLink(link),
		m_Latency(33),
		m_HostScore(200),
		m_AckAggregateTime(8),
		m_Mtu(MM_MAX_FRAGMENTSIZE),
		m_ResendLatencyMultiplier(MM_MIN_RESEND_LATENCY_MP)
	{
	}
}
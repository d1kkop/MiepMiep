#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableNewSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewSend; }

		void dispatchRelNewestQueueIfTimePassed ( u64 time );
		void dispatchRelNewestAck( u64 time );
	};
}

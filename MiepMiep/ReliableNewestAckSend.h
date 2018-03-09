#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableNewestAckSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewestAckSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewAckSend; }
	};
}

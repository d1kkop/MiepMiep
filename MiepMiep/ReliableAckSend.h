#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableAckSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableAckSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableAckSend; }
	};
}

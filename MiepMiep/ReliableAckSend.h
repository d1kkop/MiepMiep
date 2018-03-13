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
	};
}

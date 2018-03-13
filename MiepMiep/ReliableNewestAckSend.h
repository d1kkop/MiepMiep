#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;

	class ReliableNewestAckSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewestAckSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewAckSend; }
	};
}

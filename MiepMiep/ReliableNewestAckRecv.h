#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;

	class ReliableNewestAckRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewestAckRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewAckRecv; }

		void receive( class BinSerializer& bs );
	};
}

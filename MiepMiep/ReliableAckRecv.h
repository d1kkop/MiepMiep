#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;


	class ReliableAckRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableAckRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableAckRecv; }

		void receive( class BinSerializer& bs );
	};
}

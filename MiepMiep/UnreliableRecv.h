#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;

	class UnreliableRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		UnreliableRecv(Link& link);
		static EComponentType compType() { return EComponentType::UnreliableRecv; }

		void receive( class BinSerializer& bs );
	};
}

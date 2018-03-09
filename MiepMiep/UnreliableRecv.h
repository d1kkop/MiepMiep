#pragma once

#include "Link.h"


namespace MiepMiep
{
	class UnreliableRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		UnreliableRecv(Link& link);
		static EComponentType compType() { return EComponentType::UnreliableRecv; }

		void receive( class BinSerializer& bs );
	};
}

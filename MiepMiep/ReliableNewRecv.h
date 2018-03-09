#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableNewRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewRecv; }

		void receive( BinSerializer& bs );
	};
}

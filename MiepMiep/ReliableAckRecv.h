#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableAckRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableAckRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableAckRecv; }

		void receive( class BinSerializer& bs );
	};
}

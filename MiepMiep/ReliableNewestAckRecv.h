#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableNewestAckRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewestAckRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewAckRecv; }

		void receive( class BinSerializer& bs );
	};
}

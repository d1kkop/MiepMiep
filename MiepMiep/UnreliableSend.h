#pragma once

#include "Link.h"


namespace MiepMiep
{
	class UnreliableSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		UnreliableSend(Link& link);
		static EComponentType compType() { return EComponentType::UnreliableSend; }
	};
}

#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;


	class UnreliableSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		UnreliableSend(Link& link);
		static EComponentType compType() { return EComponentType::UnreliableSend; }
	};
}

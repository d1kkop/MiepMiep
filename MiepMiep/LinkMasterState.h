#pragma once

#include "ParentLink.h"
#include "Component.h"
#include "Memory.h"


namespace MiepMiep
{
	enum class ELinkMasterState : byte
	{
		Fine,
		Confused
	};


	class LinkMasterState: public ParentLink, public IComponent, public ITraceable
	{
	public:
		LinkMasterState(Link& link);
		static EComponentType compType() { return EComponentType::LinkMasterState; }

	};
}

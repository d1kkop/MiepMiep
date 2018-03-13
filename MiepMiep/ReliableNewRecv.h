#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;


	class ReliableNewRecv: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewRecv(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewRecv; }

		void receive( BinSerializer& bs );
	};
}

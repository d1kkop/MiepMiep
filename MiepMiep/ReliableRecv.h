#pragma once

#include "Component.h"
#include "Memory.h"


namespace MiepMiep
{
	class ReliableRecv: public IComponent, public ITraceable
	{
	public:
		static EComponentType compType() { return EComponentType::ReliableRecv; }

		
	};
}

#pragma once

#include "Component.h"
#include "Memory.h"


namespace MiepMiep
{
	class UnreliableRecv: public IComponent, public ITraceable
	{
	public:
		static EComponentType compType() { return EComponentType::UnreliableRecv; }
	};
}
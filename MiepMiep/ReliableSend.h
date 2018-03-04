#pragma once

#include "Component.h"
#include "Memory.h"
#include "Link.h"


namespace MiepMiep
{
	class ReliableSend: public Parent<Link>, public IComponent, public ITraceable
	{
	public:
		ReliableSend(Link& link):
			Parent(link) { }
		static EComponentType compType() { return EComponentType::ReliableSend; }

		MM_TS BinSerializer& beginSend();
	};
}

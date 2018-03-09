#pragma once

#include "Link.h"


namespace MiepMiep
{
	class ReliableSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableSend(Link& link):
			ParentLink(link) { }
		static EComponentType compType() { return EComponentType::ReliableSend; }

		MM_TS void enqueue( BinSerializer& bs, bool relay, class IDeliveryTrace* trace );

		MM_TS BinSerializer& beginSend();
	};
}

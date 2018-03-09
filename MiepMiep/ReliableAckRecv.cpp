#pragma once

#include "ReliableAckRecv.h"


namespace MiepMiep
{

	ReliableAckRecv::ReliableAckRecv(Link& link):
		ParentLink(link)
	{
	}

	void ReliableAckRecv::receive(class BinSerializer& bs)
	{

	}

}

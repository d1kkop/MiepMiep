#pragma once

#include "ReliableNewestAckRecv.h"

namespace MiepMiep
{

	ReliableNewestAckRecv::ReliableNewestAckRecv(Link& link):
		ParentLink(link)
	{

	}

	void ReliableNewestAckRecv::receive(class BinSerializer& bs)
	{

	}

}

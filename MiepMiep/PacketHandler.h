#pragma once

#include "Network.h"


namespace MiepMiep
{
	class IPacketHandler: public virtual ParentNetwork, public ITraceable
	{
	public:
		IPacketHandler(Network& network);

		void handle( const sptr<const class ISocket>& bs );
	};
}

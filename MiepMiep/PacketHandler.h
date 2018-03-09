#pragma once

#include "Network.h"


namespace MiepMiep
{
	class IPacketHandler: public virtual ParentNetwork, public ITraceable
	{
	public:
		IPacketHandler(Network& network);

		// All packets are handled here first and then if 'valid' passed to handleSpecial.
		void handle( const sptr<const class ISocket>& sock );

		virtual void handleSpecial( class BinSerializer& bs, const class Endpoint& etp ) = 0;
	};
}

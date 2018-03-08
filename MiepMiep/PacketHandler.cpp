#pragma once

#include "PacketHandler.h"
#include "Common.h"
#include "Endpoint.h"
#include "Socket.h"


namespace MiepMiep
{
	IPacketHandler::IPacketHandler(Network& network):
		ParentNetwork(network)
	{

	}

	void IPacketHandler::handle(const sptr<const class ISocket>& sock)
	{
		byte buff[MM_MAX_RECVSIZE];
		u32 rawSize = MM_MAX_RECVSIZE; // buff size in
		Endpoint etp;
		i32 err;
		ERecvResult res = sock->recv( buff, rawSize, etp, &err );

		if ( err != 0 ) 
		{
			LOGW( "Socket recv error: %d.", err );
			return;
		}

		if ( res == ERecvResult::Succes )
		{

		}
	}

}

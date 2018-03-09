#pragma once

#include "PacketHandler.h"
#include "BinSerializer.h"
#include "Common.h"
#include "Endpoint.h"
#include "Socket.h"
#include "PerThreadDataProvider.h"


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
			// Minimum size is linkId + some protocol (1 byte)
			if ( rawSize >= MM_MIN_HDR_SIZE )
			{
				BinSerializer& bs = PerThreadDataProvider::getSerializer( false );
				bs.resetTo( buff, rawSize, rawSize );
				handleSpecial( bs, etp );
			}
			else
			{
				LOGW( "Received packet with less than %d bytes. Packet discarded.", MM_MIN_HDR_SIZE );
			}
		}
	}

}

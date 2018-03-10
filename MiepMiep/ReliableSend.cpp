#include "ReliableSend.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{

	void ReliableSend::enqueue(BinSerializer& bs, bool relay, class IDeliveryTrace* trace)
	{

	}

	MM_TS BinSerializer& ReliableSend::beginSend()
	{
		auto& bs = PerThreadDataProvider::getSerializer();
		bs.write(m_Link.id());
		bs.write((byte)ReliableSend::compType());
		return bs;
	}

	MM_TS ESendCallResult priv_send_rpc(INetwork& nw, const char* rpcName, BinSerializer& bs, const IEndpoint* specific, 
										bool exclude, bool buffer, bool relay, byte channel, IDeliveryTrace* trace)
	{
		BinSerializer bsCopy;
		__CHECKEDSR( bsCopy.write( string(rpcName) ) );
		__CHECKEDSR( bsCopy.write( bs.data(), (u16)bs.length() ) );
		return nw.sendReliable( bsCopy, specific, exclude, buffer, relay, channel, trace );
	}
}
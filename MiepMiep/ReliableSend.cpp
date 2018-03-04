#include "ReliableSend.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{

	MM_TS BinSerializer& ReliableSend::beginSend()
	{
		auto& bs = PerThreadDataProvider::getSerializer();
		bs.reset();
		bs.write(m_Parent.id());
		bs.write((byte)ReliableSend::compType());
		return bs;
	}

}
#include "ReliableAckRecv.h"
#include "ReliableSend.h"
#include "Link.h"
#include "Platform.h"


namespace MiepMiep
{

	ReliableAckRecv::ReliableAckRecv(Link& link):
		ParentLink(link)
	{
	}

	MM_TS void ReliableAckRecv::receive(BinSerializer& bs) const
	{
		static thread_local vector<u32> receivedSequences;
		receivedSequences.clear();
		while ( bs.getRead() != bs.getWrite() )
		{
			u32 seq;
			__CHECKED( bs.read( seq ) );
			receivedSequences.emplace_back( seq );
		}

		auto rs = m_Link.get<ReliableSend>( this->m_Idx );
		if ( rs )
		{
			rs->ackList( receivedSequences );
		}
	}

}

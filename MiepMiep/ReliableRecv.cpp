#include "ReliableRecv.h"
#include "PacketHandler.h"
#include "JobSystem.h"


namespace MiepMiep
{

	ReliableRecv::ReliableRecv(Link& link):
		ParentLink(link),
		m_RecvSequence(0)
	{
	}

	MM_TS void ReliableRecv::receive(BinSerializer& bs, const PacketInfo& pi)
	{
		scoped_lock lk(m_RecvMutex);

		// Early out if out of data.
		if ( !PacketHelper::isSeqNewer( pi.m_Sequence, m_RecvSequence ) )
		{
			return;
		}

		// Identifies datatype
		byte packId;
		__CHECKED( bs.read(packId) );

		if ( (pi.m_Flags & MM_FRAGMENT_FIRST_BIT)!=0 && (pi.m_Flags & MM_FRAGMENT_LAST_BIT)!=0 ) // not fragmented
		{
			// Packet may arrive multiple time as recv_sequence is only incremented when expected sequence is received.
			m_OrderedPackets.try_emplace(
				pi.m_Sequence, /* key */
					Packet(packId, bs.data()+bs.getRead(), bs.getWrite()-bs.getRead(), pi.m_Flags), 1 /* value */
			);
		}
		else // fragmented
		{
			// Fragment may arrive multiple time as recv_sequence is only incremented when expected sequence is received.
			pair<decltype(m_OrderedFragments.begin()),bool> inserted = 
				m_OrderedFragments.try_emplace( pi.m_Sequence, packId, bs.data()+bs.getRead(), bs.getWrite()-bs.getRead(), pi.m_Flags );
			if ( !inserted.second )
				return; // Already exists, nothing to do.
			
			// This fragment may have completed the puzzle. Check if all fragments are available to re-assmble big packet.
			Packet finalPack;
			u32 seqBegin, seqEnd;
			if (PacketHelper::tryReassembleBigPacket(finalPack, m_OrderedFragments, pi.m_Sequence, seqBegin, seqEnd))
			{
				m_OrderedPackets.try_emplace( seqBegin, /* key */
											  move(finalPack), 1+(seqEnd-seqBegin) /* value */
				);
			}
		}

		// Process packet reply's that do not need to await game thread intervention
		auto rr = this->ptr<ReliableRecv>();
		auto& queue = m_OrderedPackets;
		m_Link.m_Network.get<JobSystem>()->addJob( [&, rr]()
		{
			scoped_lock lk(rr->m_RecvMutex);
			auto packIt = queue.find(rr->m_RecvSequence);
			while ( packIt != queue.end() )
			{
				Packet pack = move( packIt->second.first );
				u32 numFragments = packIt->second.second;
				queue.erase( packIt );
				if ( pack.m_Id == 0 ) // TODO should be if RPC
				{
					// TODO impl rpc call
				}
				rr->m_RecvSequence += numFragments;
				// try find next ordered sequence
				packIt = queue.find(rr->m_RecvSequence);
			}
		});
	}

}
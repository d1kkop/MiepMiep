#include "Link.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "Listener.h"
#include "ReliableRecv.h"
#include "UnreliableRecv.h"
#include "ReliableNewRecv.h"
#include "ReliableAckRecv.h"
#include "ReliableNewestAckRecv.h"


namespace MiepMiep
{

	Link::Link(Network& network):
		ParentNetwork(network)
	{
	}

	sptr<Link> Link::create(Network& network, const IEndpoint& other)
	{
		sptr<Link> link = reserve_sp<Link, Network&>(MM_FL, network);
		link->m_RemoteEtp = other.getCopy();
		link->m_Id = rand();
		return link;
	}

	void Link::setOriginator(const Listener& listener)
	{
		scoped_spinlock lk(m_OriginatorMutex);
		// Should only be set ever once.
		assert( !m_Originator );
		m_Originator = listener.to_ptr();
	}

	const Listener* Link::getOriginator() const
	{
		scoped_spinlock lk(m_OriginatorMutex);
		return m_Originator.get();
	}

	MM_TS void Link::createGroup(const string& typeName, const BinSerializer& initData)
	{
		auto& varVec = PerThreadDataProvider::getConstructedVariables();
		getOrAdd<GroupCollectionLink>()->addNewPendingGroup( varVec, typeName, initData, nullptr ); // makes copy
		varVec.clear();
	}

	MM_TS void Link::destroyGroup(u32 id)
	{
		
	}

	MM_TS void Link::receive(BinSerializer& bs)
	{
		byte streamId;
		PacketInfo pi;

		__CHECKED( bs.read(streamId) );
		__CHECKED( bs.read(pi.m_Flags) );
		__CHECKED( bs.read(pi.m_Sequence) );

		byte channel = pi.m_Flags & MM_CHANNEL_MASK;
		EComponentType et = (EComponentType) streamId;

		switch ( et )
		{
		case EComponentType::ReliableRecv:
			getOrAdd<ReliableRecv>(channel)->receive( bs, pi );
			break;

		case EComponentType::UnreliableRecv:
			getOrAdd<UnreliableRecv>(channel)->receive( bs );
			break;

		case EComponentType::ReliableNewRecv:
			getOrAdd<ReliableNewRecv>()->receive( bs );
			break;

			// -- Acks --

		case EComponentType::ReliableAckRecv:
			getOrAdd<ReliableAckRecv>()->receive( bs );
			break;

		case EComponentType::ReliableNewAckRecv:
			getOrAdd<ReliableNewestAckRecv>()->receive( bs );
			break;
		}
	}

}
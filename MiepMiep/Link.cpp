#include "Link.h"
#include "GroupCollection.h"
#include "PerThreadDataProvider.h"
#include "Listener.h"
#include "ReliableRecv.h"
#include "UnreliableRecv.h"
#include "ReliableNewRecv.h"
#include "ReliableAckRecv.h"
#include "ReliableNewestAckRecv.h"
#include "Util.h"


namespace MiepMiep
{

	Link::Link(Network& network):
		ParentNetwork(network)
	{
	}

	sptr<Link> Link::create(Network& network, const IEndpoint& other)
	{
		sptr<ISocket> sock = ISocket::create();
		if (!sock) return nullptr;

		i32 err;
		sock->open(IPProto::Ipv4, false, &err);
		if ( err != 0 )
		{
			LOGW( "Socket open error %d, cannot create link.", err );
			return nullptr;
		}

		sock->bind(0, &err);
		if ( err != 0 )
		{
			LOGW( "Socket bind error %d, cannot create link.", err );
			return nullptr;
		}

		sptr<Link> link = reserve_sp<Link, Network&>(MM_FL, network);
		link->m_RemoteEtp = other.getCopy();
		link->m_Id = rand();
		link->m_Socket = sock;

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

	MM_TS void Link::send(const NormalSendPacket& pack)
	{
		if ( !m_Socket )
		{
			LOGC( "Invalid socket, cannot send." );
			return;
		}

		assert( pack.m_PayLoad.length()+4 <= MM_MAX_SENDSIZE );

		// The linkID is specific to each id, all other data in the packet is shared by all links.
		// Before send, print linkID after a copy of the data.
		byte finalData[MM_MAX_SENDSIZE];
		*(u32*)finalData = Util::htonl(m_Id);
		Platform::memCpy( finalData + 4, MM_MAX_SENDSIZE-4, pack.m_PayLoad.data(), pack.m_PayLoad.length() );

		i32 err;
		ESendResult res = m_Socket->send( static_cast<const Endpoint&>( *m_RemoteEtp ), 
										  finalData,
										  pack.m_PayLoad.length()+4, 
										  &err );

		if ( err != 0 && ESendResult::Error==res ) /* ignore err if socket gets closed */
		{
			LOGW( "Socket send error %d.", err );
		}
	}

}
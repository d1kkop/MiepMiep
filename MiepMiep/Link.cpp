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

	Link::~Link()
	{
		close();
	}

	MM_TS void Link::close()
	{
		scoped_spinlock lk(m_OriginatorMutex);
		if ( m_Originator )
		{
			// For this , we make an exception, cast to non-const to reduce num clients on listeners by one
			const_pointer_cast<Listener>( m_Originator )->reduceNumClientsByOne();
			m_Originator.reset();
		}
	}

	sptr<Link> Link::create(Network& network, const IEndpoint& other, u32* id)
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
		if ( id ) 
			link->m_Id = *id;
		else
			link->m_Id = rand();
		link->m_Socket = sock;

		LOG( "Created new link to %s with id %d.", link->m_RemoteEtp->toIpAndPort().c_str(), link->m_Id );

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
		byte compType;
		PacketInfo pi;

		__CHECKED( bs.read(pi.m_Sequence) );
		__CHECKED( bs.read(compType) );
		__CHECKED( bs.read(pi.m_ChannelAndFlags) );
		

		byte channel = pi.m_ChannelAndFlags & MM_CHANNEL_MASK;
		EComponentType et = (EComponentType) compType;

		switch ( et )
		{
		case EComponentType::ReliableSend:
			getOrAdd<ReliableRecv>(channel)->receive( bs, pi );
			break;

		case EComponentType::UnreliableSend:
			getOrAdd<UnreliableRecv>(channel)->receive( bs );
			break;

		case EComponentType::ReliableNewSend:
			getOrAdd<ReliableNewRecv>()->receive( bs );
			break;

			// -- Acks --

		case EComponentType::ReliableAckSend:
			getOrAdd<ReliableAckRecv>()->receive( bs );
			break;

		case EComponentType::ReliableNewAckSend:
			getOrAdd<ReliableNewestAckRecv>()->receive( bs );
			break;

		default:
			LOGW( "Unknown stream type %d detected. Packet ignored.", (u32)et );
			break;
		}
	}

	MM_TS void Link::send(const byte* data, u32 length)
	{
		if ( !m_Socket )
		{
			LOGC( "Invalid socket, cannot send." );
			return;
		}

		i32 err = 0;
		ESendResult res = m_Socket->send( static_cast<const Endpoint&>( *m_RemoteEtp ), data, length, &err );

		if ( err != 0 && ESendResult::Error==res ) /* ignore err if socket gets closed */
		{
			LOGW( "Socket send error %d.", err );
		}
	}

}
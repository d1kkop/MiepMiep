#pragma once

#include "UnitTestBase.h"
#include "Socket.h"
#include "SocketSet.h"
#include "Listener.h"
#include "Endpoint.h"
#include "ListenerManager.h"

struct Sequence
{
	u32* values;
	u32 sent;
	u32 received;

	Sequence(): values( nullptr ) { }
	~Sequence()
	{
		delete [] values;
	}
};

struct ConnectSeq
{
	Sequence sequences[8];
	wptr<ILink> m_recvLink;
	wptr<ILink> m_sendLink;
};

constexpr u32 numClients=10;
ConnectSeq g_cqs[numClients];
mutex g_mt;


MM_RPC( ReliableRpc, u32 )
{
	scoped_lock lk(g_mt);
	u32 val = get<0>(tp);

	for ( auto& cs : g_cqs )
	{
		auto sl = cs.m_recvLink.lock();
		if ( sl.get() == link )
		{
			auto& s = cs.sequences[ channel ];
			u32 wanted = s.values[s.received];
			assert( wanted == val );
			if ( wanted != val ) cout << "INVALID RELIABLE ORDER VALUE" << endl;
			s.received++;
			break;
		}
	}
}


UTESTBEGIN(ReliableTest)
{
	constexpr u32 numPacks = 100000;

	sptr<INetwork> nw = INetwork::create( false );
	wptr<INetwork> n  = nw;
//	nw->simulatePacketLoss( 40 );

	 Network& net = toNetwork(*nw);
	 sptr<LinkManager> lm = net.getOrAdd<LinkManager>();

	 net.startListen( 12345 );
	 sptr<Listener> listener = net.getOrAdd<ListenerManager>()->findListener( 12345 );

	 sptr<Session> hostSes   = reserve_sp<Session>( MM_FL, net, MetaData() );
//	 sptr<ISocket> hostSock  = ISocket::create( 12345 );
	 sptr<ISocket>  clientSocks[numClients];
	 sptr<IAddress> clientAddr[numClients];
	 sptr<Session>  clientSes[numClients];
	 SocketAddrPair saps[numClients];
	 
	 for ( u32 i=0; i < numClients; i++ )
	 {
		 clientSocks[i] = ISocket::create( 0 );
		 clientAddr[i]  = IAddress::resolve( "localhost", 12345 );
		 clientSes[i]   = reserve_sp<Session>( MM_FL, net, MetaData() );
		 saps[i] = SocketAddrPair( clientSocks[i], clientAddr[i] );
		 for (auto& s : g_cqs[i].sequences)
		 {
			 s.sent = 0;
			 s.received = 0;
			 s.values = new u32[numPacks];
			 for ( u32 j=0; j<numPacks; j++ )
			 {
				 s.values[j] = Util::rand();
			 }
		 }

		 g_cqs[i].m_recvLink = lm->add( *clientSes[i], saps[i], 200+i, true, true );
		 u16 boundPort = 0;
		 boundPort = Endpoint::createSource( *clientSocks[i] )->port();
		 g_cqs[i].m_sendLink = lm->add( sc<SessionBase&>( *hostSes ),
										SocketAddrPair( listener->socket().to_sptr(), IAddress::resolve( "localhost", boundPort ) ),
										200+i, false, true );
	 }

	
	 bool bContinue = true;
	 while ( bContinue )
	 {
		 u32 ct = Util::rand() % numClients;
		 u32 mch = 8;
		 u32 ch =  Util::rand() % mch;
			 
		if ( g_cqs[ct].sequences[ch].sent != numPacks )
		{
			sptr<Link> ll = static_pointer_cast<Link>( g_cqs[ct].m_sendLink.lock() );
			ll->callRpc<ReliableRpc, u32>( 
				g_cqs[ct].sequences[ch].values[g_cqs[ct].sequences[ch].sent++], No_Local, No_Relay, (byte)ch
				);
		}

		 bContinue = false;
		 
		 for ( auto & g_cq : g_cqs )
		 {
			 u32 k=0;
			 for ( auto & sequence : g_cq.sequences )
			 {
				 if ( sequence.received != numPacks )
				 {
					 bContinue = true;
					 break;
				 }
				 if ( ++k == mch )
					 break;
			 }
		 }

		 nw->processEvents();
		// std::this_thread::sleep_for( chrono::milliseconds( 2 ) );
	 }

	return true;
}
UNITTESTEND( ReliableTest )
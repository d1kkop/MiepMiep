#pragma once

#include "UnitTestBase.h"
#include "Socket.h"
#include "SocketSet.h"


u32* g_sequence[8];
u32 g_sendSeq[8];


MM_RPC( ReliableRpc, u32 )
{
}


UTESTBEGIN(ReliableTest)
{
	sptr<INetwork> nw = INetwork::create( true );
	wptr<INetwork> n = nw;

	 Network& net = toNetwork(*nw);
	 sptr<LinkManager> lm = net.getOrAdd<LinkManager>();
	 sptr<Session> sesA = reserve_sp<Session>( MM_FL, net, MetaData() );
	 sptr<Session> sesB = reserve_sp<Session>( MM_FL, net, MetaData() );
	 
	 u32 numPacks = 100;
	 for ( u32 i=0; i<8; i++ )
	 {
		 g_sendSeq[i] = 0;
		 g_sequence[i] = new u32[numPacks];
		 for ( u32 j=0; j<numPacks; j++ )
		 {
			 g_sequence[i][j] = Util::rand();
		 }
	 }

	 sptr<ISocket> sockA = ISocket::create(12345);
	 sptr<ISocket> sockB = ISocket::create(12346);

	 sptr<IAddress> toA = IAddress::resolve( "localhost", 12346 );
	 sptr<IAddress> toB = IAddress::resolve( "localhost", 12345 );

	 SocketAddrPair saps[] = {
		 SocketAddrPair( sockA, toA ),
		 SocketAddrPair( sockB, toB )
	 };

	 lm->add( *sesA, saps[0], 222, true );
	 lm->add( *sesB, saps[1], 222, true );

	 bool bContinue = true;
	 while ( bContinue )
	 {
		 u32 lr = Util::rand() % 2;
		 u32 ch = Util::rand() % 8;
		 if ( g_sendSeq[ch] != numPacks )
		 {
			 lm->get( saps[lr] )->callRpc< ReliableRpc, u32 >
				 (
					 g_sequence[ch][g_sendSeq[ch]++], No_Local, No_Relay, (byte)ch 
				 );
		 }

		 bContinue = false;
		 for ( u32 i=0; i <8; i++ )
		 {
			 if ( g_sendSeq[i] != numPacks )
			 {
				 bContinue = true;
				 break;
			 }
		 }
	 }

	std::this_thread::sleep_for( chrono::seconds(4) );
	for ( u32 i=0; i<8; i++ )
		delete [] g_sequence[i];

	return false;
}
UNITTESTEND( ReliableTest )
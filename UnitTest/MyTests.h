#pragma once

#include "UnitTestBase.h"
#include "Socket.h"
#include "SocketSet.h"
#include <thread>
#include <mutex>
#include <cassert>
using namespace std;


UTESTBEGIN(SocketTest)
{
	sptr<ISocket> sock = ISocket::create();
	i32 err;
	assert( sock && "sock create" );
	assert( sock->open( IPProto::Ipv4, true, &err ) && "sock open" );
	assert( sock->bind( 27001, &err ) && "sock bind" );

	// should not be rebindable
	sptr<ISocket> sock2 = ISocket::create();
	assert( sock2 && "sock create" );
	assert( sock2->open( IPProto::Ipv4, false, &err ) && "sock open" );
	assert( !sock2->bind( 27001, &err ) && "sock bind" );
	assert( sock2->isOpen() && !sock2->isBound() ); // should be open but not bound

	// should be rebindable
	sptr<ISocket> sock3 = ISocket::create();
	assert( sock3 && "sock create" );
	assert( sock3->open( IPProto::Ipv4, true, &err ) && "sock open" );
	assert( sock3->bind( 27001, &err ) && "sock bind" );

	sptr<ISocket> sock4 = ISocket::create();
	assert( sock4 && "sock create" );
	assert( sock4->open( IPProto::Ipv4, false, &err ) && "sock open" );
	assert( sock4->bind( 27005, &err ) && "sock bind" );
	assert( sock4->isOpen() && sock4->isBound() && "not both bound and open");

	sock4->close();
	assert( !sock4->isOpen() && !sock4->isBound() && "socket should be closed" );
	return true;
}
UNITTESTEND(SocketTest)


UTESTBEGIN(SocketSetTest)
{
	// ensures socket environment gets initialized
	sptr<ISocket> sock = ISocket::create();

	SocketSet ss2;
	i32 err;
	EListenOnSocketsResult res = ss2.listenOnSockets(1, [&](auto& s) { }, &err );
	assert( res == EListenOnSocketsResult::NoSocketsInSet );

	constexpr auto kThreads=50;
	i32 k=10000;
	thread threads[kThreads];
	SocketSet ss[kThreads];
	i32 j=0;
	for (auto& t : threads)
	{
		t = thread( [&,j]()
		{
			while (k-->0)
			{
				sptr<ISocket> sock = ISocket::create();
				sock->open();
				sock->bind(0);
				ss[rand()%4].addSocket( sock );
				i32 err;
				EListenOnSocketsResult res = ss[j].listenOnSockets(1, [&]( sptr<ISocket>& rdySocket )
				{
					cout << "Socket is ready." << endl;
				}, &err);
				assert ( res == EListenOnSocketsResult::TimeoutNoData || res == EListenOnSocketsResult::Fine || res == EListenOnSocketsResult::NoSocketsInSet );
			}
		});
		j++;
	}
	
	for ( auto& t : threads )
	{
		if ( t.joinable() )
			t.join();
	}

	return true;
}
UNITTESTEND(SocketSetTest)

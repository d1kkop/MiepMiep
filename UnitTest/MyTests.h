#pragma once

#include "UnitTestBase.h"
#include "Socket.h"
#include "Memory.h"
#include "BinSerializer.h"
#include "Threading.h"
#include "PerThreadDataProvider.h"
#include "Util.h"
#include "LinkManager.h"
#include "Network.h"
#include "MiepMiep.h"
#include "MasterServer.h"
#include "SocketSetManager.h"
#include "Common.h"
#include <thread>
#include <mutex>
#include <cassert>
using namespace std;
using namespace chrono;
using namespace MiepMiep;


void registerResult( const ISession& session, bool result )
{
}


void joinResult( const ISession& session, bool result )
{
}

UTESTBEGIN(SocketTest)
{
	sptr<ISocket> sock = ISocket::create();
	i32 err=0;
	assert( sock && "sock create" );
	assert( sock->open( IPProto::Ipv4, SocketOptions(), &err ) && "sock open" );
	assert( sock->bind( 27001, &err ) && "sock bind" );

	// should not be rebindable
	sptr<ISocket> sock2 = ISocket::create();
	assert( sock2 && "sock create" );
	assert( sock2->open( IPProto::Ipv4, SocketOptions(), &err ) && "sock open" );
	assert( !sock2->bind( 27001, &err ) && "sock bind" );
	assert( sock2->isOpen() && !sock2->isBound() ); // should be open but not bound

	// open new one with reusable address
	sptr<ISocket> sock3 = ISocket::create();
	assert( sock3 && "sock create" );
	assert( sock3->open( IPProto::Ipv4, SocketOptions(true), &err ) && "sock open" );
	assert( sock3->bind( 27002, &err ) && "sock bind" );

	// should be rebindable
	for ( auto i=0; i<5; i++ )
	{
		sptr<ISocket> sock4 = ISocket::create();
		assert( sock4 && "sock create" );
		assert( sock4->open( IPProto::Ipv4, SocketOptions(true), &err ) && "sock open" );
		assert( sock4->bind( 27002, &err ) && "sock bind" );
		assert( sock4->isOpen() && sock4->isBound() && "not both bound and open");
		sock4->close();
	}

	sptr<ISocket> sock5 = ISocket::create();
	assert(sock5 && "sock create");
	assert(sock5->open(IPProto::Ipv4, SocketOptions(true), &err) && "sock open");
	assert(sock5->bind(27003, &err) && "sock bind");
	assert(sock5->isOpen() && sock5->isBound() && "not both bound and open");
	sock5->close();

	assert( !sock5->isOpen() && !sock5->isBound() && "socket should be closed" );
	return true;
}
UNITTESTEND(SocketTest)


UTESTBEGIN(SocketSetTest)
{
	// ensures socket environment gets initialized
	sptr<ISocket> sock = ISocket::create();
	sptr<INetwork> network = INetwork::create();

	Network& nw = toNetwork( *network ) ;

	constexpr auto kThreads=50;
	i32 k=10000;
	thread threads[kThreads];
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
				nw.getOrAdd<SocketSetManager>()->addSocket( sock );
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


UTESTBEGIN(SpinLockTest)
{
//	return true;
	SpinLock sl;
	u32 num=0;
	const u32 kThreads=100;
	const u32 mp=1000;
	thread tds[kThreads];
	for (auto & td : tds)
	{
		td = thread( [&]()
		{
			for ( u32 j=0; j<mp; j++)
			{
				scoped_spinlock lk(sl);
				num += 1;
			}
		});
	}
	for (auto& t : tds)
		t.join();

	assert( num == kThreads*mp );
	return (num == kThreads*mp);
}
UNITTESTEND(SpinLockTest)

UTESTBEGIN( SpinLockVsNormalLock )
{
//	return true;
	SpinLock sl;
	mutex mt;

	constexpr u32 kThreads[] = { 100, 50, 25, 8, 4 };

	for (unsigned long kThread : kThreads)
	{
	// -------------------------------------------------------
		cout << "Spinlock Add with " << kThread << " threads" << endl;
		u32 num=0;
		const u32 mp=1000;
		thread tds[100];
		u64 t1 = Util::abs_time();
		for ( u32 i=0; i < kThread; i++)
		{
			thread& td = tds[i];
			td = thread( [&] ()
			{
				for ( u32 j=0; j<mp; j++ )
				{
					scoped_spinlock lk( sl );
					num += 1;
				}
			} );
		}
		for ( u32 i=0; i < kThread; i++ )
			tds[i].join();
		u64 t2 = Util::abs_time();
		assert( num == kThread*mp );
		cout << "Took " << (t2-t1) << " ms" << endl;

		// -------------------------------------------------------
		cout << "Mutex Add with " << kThread << " threads" << endl;
		num=0;
		t1 = Util::abs_time();
		for ( u32 i=0; i < kThread; i++ )
		{
			thread& td = tds[i];
			td = thread( [&] ()
			{
				for ( u32 j=0; j<mp; j++ )
				{
					scoped_lock lk( mt );
					num += 1;
				}
			} );
		}
		for ( u32 i=0; i < kThread; i++ )
			tds[i].join();
		t2 = Util::abs_time();
		assert( num == kThread*mp );
		cout << "Took " << (t2-t1) << " ms" << endl;

		// -------------------------------------------------------
		cout << "Atomic Add with " << kThread << " threads" << endl;
		num=0;
		atomic<u32> aNum=0;
		t1 = Util::abs_time();
		for ( u32 i=0; i < kThread; i++ )
		{
			thread& td = tds[i];
			td = thread( [&] ()
			{
				for ( u32 j=0; j<mp; j++ )
				{
					aNum += 1;
				}
			} );
		}
		for ( u32 i=0; i < kThread; i++ )
			tds[i].join();
		t2 = Util::abs_time();
		assert( aNum == kThread*mp );
		cout << "Took " << (t2-t1) << " ms" << endl;
	}
	
	return true;
}
UNITTESTEND( SpinLockVsNormalLock )


RecvPacket PackProvider()
{
	auto& bs = PerThreadDataProvider::getSerializer(true);
	bs.write(string("2 hello, how are you? 2"));
	return RecvPacket( 6, bs );
}

UTESTBEGIN(PacketTest)
{
	//return true;
	constexpr u32 kThreads = 50;
	thread tds[kThreads];

	for ( auto& t : tds )
	{
		t = thread( [] ()
		{
			auto& bs = PerThreadDataProvider::getSerializer(true);
			bs.write(string("hello, how are you?"));
		//	cout << "p1" << endl;
			RecvPacket p1( 7, bs );
		//	cout << "p2" << endl;
			RecvPacket p2(p1);
		//	cout << "p3" << endl;
			RecvPacket p3 = move( PackProvider() ); //Packet( 7, bs ); // move( Packet( 7, bs ) );
			assert( string ( (char*)p3.m_Data ) == "2 hello, how are you? 2" );
	//		cout << "p4" << endl;
			RecvPacket p4( move( RecvPacket( 7, bs ) ) );
		});
	}

	for ( auto& t : tds ) t.join();

	return true;
}
UNITTESTEND(PacketTest)


UTESTBEGIN(TimeTest)
{
//	return true;
	constexpr u32 kThreads = 50;
	thread tds[kThreads];

	for ( auto& t : tds )
	{
		t = thread( [] ()
		{
			u64 t = Util::abs_time();
			for ( u32 i=0; i < 1000; i++ )
			{
				this_thread::sleep_for( milliseconds(1) );
			}
			u64 t2 = Util::abs_time();
			u64 d  = t2-t;
			if ( !( d >= 940 && d <= 1060 ) )
			{
				cout << "WARNING: Thread waited for " << d << " ms, this is VERY inaccurate." << endl;
			}
		//	cout << "Thread waited for " << d << " ms." << endl;
		});
	}

	for ( auto& t : tds ) t.join();

	return true;
}
UNITTESTEND(TimeTest)


UTESTBEGIN(BinMetaDataTest)
{
//	return true;
	BinSerializer bs;
	MetaData md;

	for (u32 i = 0; i < 10000 ; i++)
	{
		md[ std::to_string(i) ] = " lalalal this is metadata johoeeeee with value: " + std::to_string(i);
	}
	bs.write( md );

	BinSerializer bs2( bs );

	MetaData mdRead;
	bs.read(mdRead);
	assert( mdRead == md );

	return true;
}
UNITTESTEND(BinMetaDataTest)


UTESTBEGIN(StringSerializeTest)
{
//	return true;
	BinSerializer bs;
	string str;
	for (u32 i = 0; i < MiepMiep::TempBuffSize-1 ; i++)
	{
		auto r = rand () % 10;
		str.append(to_string(r));
	}
	
	__CHECKEDB( bs.write(str) );

	BinSerializer bs2(bs);
	string strOut;
	__CHECKEDB( bs2.read( strOut ) );

	u64 val = ~0ULL-1000;
	bs.write( val );
	bs.moveRead( bs2.length() );
	u64 val2;
	bs.read( val2 );

	assert( val == val2 );

	assert( strOut == str );
	return true;
}
UNITTESTEND(StringSerializeTest)


// mimic pw & md
MetaData g_md;
string g_pw;
MM_RPC(MyRpcBigTest, string, MetaData)
{
	const string& pw	= std::get<0>(tp);
	const MetaData& md	= std::get<1>(tp);
	assert( md == g_md );
}

UTESTBEGIN(RPCBigTest)
{
	return true;
	BinSerializer bs;
	g_md.clear();
	for (u32 i = 0; i < 10000 ; i++)
	{
		g_md[ std::to_string(i) ] = " lalalal this is metadata johoeeeee with value: " + std::to_string(i);
	}
	bs.write( g_md );

	BinSerializer bs2;
	string pw = string("this is my pw"); 
	bs2.write( pw );
	bs2.write( bs );

	MetaData mdRead;
	bs2.moveRead( (u32)pw.length()+1 );
	bs2.read(mdRead);
	assert( mdRead == g_md );
	bs2.setRead(0);
	string pw2;
	bs2.read(pw2);
	assert(pw2 == pw);
	mdRead.clear();
	bs2.read(mdRead);
	assert(mdRead == g_md);
	g_pw = pw;

	sptr<INetwork> nw = INetwork::create();

	auto lRes = nw->startListen( 21002 );
	assert(lRes == EListenCallResult::Fine );

	nw->registerServer( [] ( auto& ses, bool succes )
	{
		
	}, *IAddress::resolve("localhost", 21002), "unitTestGame", "type", true, false, 10, 32, "" );

	nw->joinServer( [&] ( const ISession& ses, bool jres )
	{
		if ( jres )
		{
			auto res = nw->callRpc<MyRpcBigTest, string, MetaData>( g_pw, g_md, &ses, nullptr, No_Local, No_Buffer, No_Relay );
			assert( res == ESendCallResult::Fine );
		}
		else assert(false);
	}, *IAddress::resolve("localhost", 21002), "unitTestGame", "type", 0, 100, 0, 100, true, true );

	return true;
}
UNITTESTEND(RPCBigTest)


UTESTBEGIN(AutoChatServerAndClient)
{
//	return true;
	sptr<INetwork> nw = INetwork::create();
	
	nw->startListen( 27001 );

	nw->registerServer( [](auto& l, auto r) { registerResult(l, r); }, *IAddress::resolve("localhost", 27001),
						"my game", "type",
						true, false, 10, 32, "lala" );

	nw->joinServer( [](auto& l, auto r) { joinResult(l, r); }, *IAddress::resolve("localhost", 27001),
					"first game", "type", 5, 15, 0, 128, true, true );

	nw->joinServer( [](auto& l, auto r) { joinResult(l, r); }, *IAddress::resolve("localhost", 27001), 
					"first game", "type", 5, 15, 0, 128, true, true );
	nw->joinServer( [](auto& l, auto r) { joinResult(l, r); }, *IAddress::resolve("localhost", 27001), 
					"first game", "type", 5, 15, 0, 128, true, true );
	nw->joinServer( [](auto& l, auto r) { joinResult(l, r); }, *IAddress::resolve("localhost", 27001), 
					"first game", "type", 5, 15, 0, 128, true, true );

	// TODO fixup this unit test
	return true;
}
UNITTESTEND(AutoChatServerAndClient)


UTESTBEGIN( SapLookupInMap )
{
//	return true;
	sptr<ISocket> s1 = ISocket::create();
	sptr<ISocket> s2 = ISocket::create();
	sptr<ISocket> s3 = s1;
	if ( !s1->open() ) return false;
	if ( !s1->bind(0) ) return false;
	if ( !s2->open() ) return false;
	if ( !s2->bind( 23001 ) ) return false;
	sptr<IAddress> a1 = IAddress::resolve( "localhost", 23001 );
	sptr<IAddress> a2 = IAddress::resolve( "localhost", 23001 );
	sptr<IAddress> a3 = a1->getCopy();
	if ( !a1 ) return false;

	map<SocketAddrPair, int> myMap;
	myMap[ SocketAddrPair( s1, a1 ) ] = 3;
	assert( myMap.count( SocketAddrPair( s1, a1 ) ) == 1 );
	assert( myMap[ SocketAddrPair( s1, a1 ) ] == 3 );
	assert( myMap.count( SocketAddrPair( s2, a2 ) ) == 0 );
	myMap[SocketAddrPair( s2, a2 )] = 225;
	assert( myMap[SocketAddrPair( s1, a1 )] == 3 );
	assert( myMap[SocketAddrPair( s2, a2 )] == 225 );
	assert( myMap.count( SocketAddrPair( s3, a3 ) ) == 1 );
	assert( myMap[SocketAddrPair( s3, a3 )] == 3 );
	assert( myMap.count( SocketAddrPair( s1, a3 ) ) == 1 );
	assert( myMap[SocketAddrPair( s1, a3 )] == 3 );
	assert( myMap.count( SocketAddrPair( s3, a1 ) ) == 1 );
	assert( myMap[SocketAddrPair( s3, a1 )] == 3 );

	return true;
}
UNITTESTEND( SapLookupInMap )
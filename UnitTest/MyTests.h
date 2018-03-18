#pragma once

#include "UnitTestBase.h"
#include "Socket.h"
#include "SocketSet.h"
#include "PacketHandler.h"
#include "Memory.h"
#include "BinSerializer.h"
#include "Threading.h"
#include "PerThreadDataProvider.h"
#include "Util.h"
#include "LinkManager.h"
#include "Network.h"
#include "MiepMiep.h"
#include <thread>
#include <mutex>
#include <cassert>
using namespace std;
using namespace chrono;
using namespace MiepMiep;


UTESTBEGIN(SocketTest)
{
	sptr<ISocket> sock = ISocket::create();
	i32 err;
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


class UnitTestPacketHandler: public IPacketHandler
{
public:
	UnitTestPacketHandler(Network& nw):
		IPacketHandler(nw) { }

	void handleSpecial( class BinSerializer& bs, const Endpoint& etp ) override
	{

	}
};

UTESTBEGIN(SocketSetTest)
{
	// ensures socket environment gets initialized
	sptr<ISocket> sock = ISocket::create();
	sptr<INetwork> network = INetwork::create();

	SocketSet ss2;
	i32 err;
	EListenOnSocketsResult res = ss2.listenOnSockets(1, &err );
	assert( res == EListenOnSocketsResult::NoSocketsInSet );

	
	Network& nw = toNetwork( *network ) ;
	sptr<UnitTestPacketHandler> handler = reserve_sp<UnitTestPacketHandler, Network&>( MM_FL, nw );

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
				ss[rand()%4].addSocket( const_pointer_cast<const ISocket>( sock ), static_pointer_cast<IPacketHandler>( handler ) );
				i32 err;
				EListenOnSocketsResult res = ss[j].listenOnSockets(1, &err);
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


UTESTBEGIN(SpinLockTest)
{
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


RecvPacket PackProvider()
{
	auto& bs = PerThreadDataProvider::getSerializer(true);
	bs.write(string("2 hello, how are you? 2"));
	return RecvPacket( 6, bs );
}

UTESTBEGIN(PacketTest)
{
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
MM_RPC(MyRpcBigTest, string, MetaData)
{
	const string& pw	= std::get<0>(tp);
	const MetaData& md	= std::get<1>(tp);
	assert( md == g_md );
}

UTESTBEGIN(RPCBigTest)
{
	BinSerializer bs;

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

	sptr<INetwork> nw = INetwork::create();
	sptr<Link> link   = sc<Network&>(*nw).getOrAdd<LinkManager>()->add( *IAddress::resolve("localhost", 12203) );
	bs2.setRead(0);
	rpc_dsr_MyRpcBigTest( *nw, (ILink&) (Link&)*link, bs2 );

	return true;
}
UNITTESTEND(RPCBigTest)


UTESTBEGIN(AutoChatServerAndClient)
{
	sptr<INetwork> nw = INetwork::create();
	
	nw->startListen( 27001, "lala2");

	nw->registerServer( *IAddress::resolve("localhost", 27001), "myFirstServ", "lala2" );
	nw->joinServer( *IAddress::resolve("localhost", 27001), "myFirstServ", "lala2" );
	nw->joinServer( *IAddress::resolve("localhost", 27001), "myFirstServ", "lala2" );
	nw->joinServer( *IAddress::resolve("localhost", 27001), "myFirstServ", "lala2" );
	nw->joinServer( *IAddress::resolve("localhost", 27001), "myFirstServ", "lala2" );

	return true;
}
UNITTESTEND(AutoChatServerAndClient)
#include "Game.h"
#include "BinSerializer.h"
#include "Apple.h"
#include "Tree.h"
#include <iostream>
#include <tuple>
#include <thread>
#include <chrono>
using namespace std;
using namespace chrono;



namespace MyGame
{
	// Define your RPC (this is 'just' a static function)
	// Inetwork, Iendpopint and Tuple<params...> tp are passed in always.
	MM_RPC( myFirstRpc, i32, i32 )
	{
		auto var1 = get<0>(tp);
		auto var2 = get<1>(tp);

		cout << var1 << " " << var2 << endl;
	}


	MM_VARGROUP( myFirstVarGroup, string, bool )
	{
		const auto& var1 = get<0>(tp);
		auto var2 = get<1>(tp);
		cout << var1 << " " << var2 << endl;
	}

	Game::Game()
	= default;


	bool Game::init()
	{
		m_Network = INetwork::create( true );

		m_Network->addConnectionListener( this );
		
		m_Network->startListen( 27002, "my first pw" );

		/*
		m_Network->callRpc<myFirstRpc, i32, i32>( 5, 10, true );
		m_Network->createGroup<myFirstVarGroup, string, bool>( "hello bartje k", true, true, 6 );*/

		auto masterEtp = IAddress::resolve( "localhost", 27002 );

		cout << masterEtp->toIpAndPort() << endl;

		MetaData md;

		//for ( i32 i=0; i<1000; i++ )
		//{
		//	md[ to_string(i) ] = "metadata meta data meta data ta ta ata at ta taaaaa!!! " + to_string(i);
		//}

		m_Network->registerServer( *masterEtp, "myFirstGame", "my first pw", md );
		m_Network->joinServer( *masterEtp, "myFirstGame" );

		this_thread::sleep_for( milliseconds(20000) );
		return true;
	}

	void Game::run()
	{

	}

	void Game::stop()
	{
		if ( m_Network )
			m_Network->removeConnectionListener( this );
	}

	void Game::onConnectResult(const ILink& link, EConnectResult res)
	{
		switch (res)
		{
		case MiepMiep::EConnectResult::Fine:
			cout << "connected to: " << link.destination().toIpAndPort() << endl;
			break;
		case MiepMiep::EConnectResult::Timedout:
			cout << "connecting to: " << link.destination().toIpAndPort() << " timed out." << endl;
			break;
		case MiepMiep::EConnectResult::InvalidPassword:
			cout << "connecting to: " << link.destination().toIpAndPort() << " invalid password." << endl;
			break;
		case MiepMiep::EConnectResult::MaxConnectionsReached:
			cout << "connecting to: " << link.destination().toIpAndPort() << " max connections reached." << endl;
			break;
		case MiepMiep::EConnectResult::AlreadyConnected:
			cout << "connecting to: " << link.destination().toIpAndPort() << " already connected." << endl;
			break;
		default:
			cout << "connecting to: " << link.destination().toIpAndPort() << " unknown result returned!" << endl;
			break;
		}
	}
}

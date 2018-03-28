#include "Game.h"
#include "BinSerializer.h"
#include "MasterServer.h" 
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
		BinSerializer bs;

		m_Network = INetwork::create( true );
		m_Network->addConnectionListener( this );


		m_Network->startListen( 27002 );

		/*
		m_Network->callRpc<myFirstRpc, i32, i32>( 5, 10, true );
		m_Network->createGroup<myFirstVarGroup, string, bool>( "hello bartje k", true, true, 6 );*/

		auto masterEtp = IAddress::resolve( "localhost", 27002 );

		cout << masterEtp->toIpAndPort() << endl;

		MetaData md;

		//for ( i32 i = 0; i<1000; i++ )
		//{
		//	md[to_string( i )] = "metadata meta data meta data ta ta ata at ta taaaaa!!! " + to_string( i );
		//}

		MetaData serverHostMd =
		{
			{ "hostKey1", "hostValue1" },
			{ "hostKey2", "hostValue2" },
			{ "hostKey3", "hostValue3" },
		};

		MetaData serverMatchMd = 
		{
			{ "servKey1", "servValue1" },
			{ "servKey2", "servValue2" },
			{ "servKey3", "servValue3" },
		};

		m_Network->registerServer( [this](auto& l, auto r) { onRegisterResult(l, r); }, *masterEtp,
								    true, false, true, 10.f, 32, "first game", "type", "", 
									serverHostMd, serverMatchMd );

		MetaData joinMd =
		{
			{ "joinKey1", "joinValue1" },
			{ "joinKey2", "joinValue2" },
			{ "joinKey3", "joinValue3" },
		};

		MetaData joinMatchMd =
		{
			{ "joinMatchKey1", "joinMatchValue1" },
			{ "joinMatchKey2", "joinMatchValue2" },
			{ "joinMatchKey3", "joinMatchValue3" },
		};
		 
		// Note the above is an async process, so make sure it is actually registered, otherwise join will fail immediately!
		std::this_thread::sleep_for( milliseconds(20) );
		m_Network->joinServer( [this] (auto& l, auto r) { onJoinResult(l, r); }, *masterEtp,
							   "first game", "type", 5, 15, 0, 128, true, true, 
							   joinMd, joinMatchMd );

		this_thread::sleep_for( milliseconds(20000) );
		return true;
	}

	void Game::run()
	{

	}

	void Game::stop()
	{
		if ( m_Network )
		{
			m_Network->removeConnectionListener( this );
		}
	}

	void Game::onRegisterResult( const ILink& link, bool succes )
	{
		if ( succes )
		{
			cout << "Succesfully registered server at " << link.destination().toIpAndPort() << endl;
		}
		else
		{
			cout << "Failed to registered server at " << link.destination().toIpAndPort() << endl;
		}
	}

	void Game::onJoinResult( const ILink& link, EJoinServerResult res )
	{
		switch ( res )
		{
		case EJoinServerResult::Fine:
			cout << "connected to: " << link.destination().toIpAndPort() << endl;
			break;
		case EJoinServerResult::NoMatchesFound:
			cout << "no matches found " << link.destination().toIpAndPort() << endl;
		}
	}

	void Game::onConnectResult( const ILink& link, EConnectResult res )
	{
		int k = 0;
	}
}

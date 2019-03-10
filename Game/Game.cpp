#include "Game.h"
#include "BinSerializer.h"
#include "MasterSessionManager.h" 
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

		sptr<ISession> regSes = m_Network->registerServer( [this](auto& l, auto r) { onRegisterResult(l, r); }, *masterEtp,
														   "first game", "type", true, false, 10.f, 32, "",
														   serverHostMd, serverMatchMd );
		if ( regSes )
		{
			m_Network->addSessionListener( *regSes, this );
		}

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
		// 
		// Note the above is an async process, so make sure it is actually registered, otherwise join will fail immediately!
		std::this_thread::sleep_for( milliseconds(20) );
		sptr<ISession> joinSes = m_Network->joinServer( [this] (auto& l, auto r) { onJoinResult(l, r); }, *masterEtp,
							   "first game", "type", 5, 15, 0, 128, true, true, 
							   joinMd, joinMatchMd );

		if ( joinSes )
		{
			m_Network->addSessionListener( *joinSes, this );
		}

		this_thread::sleep_for( milliseconds(200000) );
		return true;
	}

	void Game::run()
	{

	}

	void Game::stop()
	{
		if ( m_Network )
		{
		//	m_Network->removeConnectionListener( this );
		}
	}

	void Game::onRegisterResult( ISession& session, bool succes )
	{
		if ( succes )
		{
			cout << "Succesfully registered server at " << session.matchMaker()->destination().toIpAndPort() << endl;
		}
		else
		{
			cout << "Failed to registered server at " << session.matchMaker()->destination().toIpAndPort() << endl;
		}
	}

	void Game::onJoinResult( ISession& session, bool res )
	{
		switch ( res )
		{
		case EJoinServerResult::Fine:
			cout << "connected to: " << session.matchMaker()->destination().toIpAndPort() << endl;
			break;
		case EJoinServerResult::NoMatchesFound:
			cout << "no matches found " << session.matchMaker()->destination().toIpAndPort() << endl;
			break;
		}
	}

	void Game::onConnect( ISession & session, const MetaData& md, const IAddress & remote )
	{
		cout << "New connection " << remote.toIpAndPort() << " in " << session.name() << endl;
	}

	void Game::onDisconnect( ISession & session, const IAddress & remote, EDisconnectReason reason )
	{
		string discReason = "disconnected.";
		if (reason == EDisconnectReason::Kicked) discReason = "kicked.";
		if (reason == EDisconnectReason::Lost) discReason = "lost.";
		cout << remote.toIpAndPort() << " in session " << session.name() << " " << discReason << endl;
	}

	void Game::onOwnerChanged( ISession & session, NetVar & variable, const IAddress * newOwner )
	{
		cout << "Variable from group with id " << variable.groupId() << " has new owner " << endl;
	}

	void Game::onNewHost( ISession & session, const IAddress * host )
	{
		cout << "New host: " << (host?host->toIpAndPort():"us") << " in session " << session.name() << endl;
	}

	void Game::onLostHost( ISession & session )
	{
		cout << "Lost host in session " << session.name() << endl;
	}

	void Game::onLostMasterLink( ISession & session, const ILink & link )
	{
		cout << "Lost link to matchmaker server in session " << session.name() << endl;
	}

}

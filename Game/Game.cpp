#include "Game.h"
#include "BinSerializer.h"
#include "Apple.h"
#include "Tree.h"
#include <iostream>
#include <tuple>
using namespace std;



namespace MyGame
{
	struct MyTest
	{
		MyTest():
			ptr(this)
		{
		}

		~MyTest()
		{
			cout << "destructing my test"<< endl;
		}

		int age;
		sptr<MyTest> ptr;
	};

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
		sptr<MyTest> mt2;
		{
			MyTest* mt = new MyTest;
			 cout << mt->ptr.use_count() << endl;
			 mt2 = mt->ptr;
			 mt2._Decref();
			 cout << mt2->ptr.use_count() << endl;
		}

	/*	cout << mt2.use_count() << endl;

		m_Network = INetwork::create();

		m_Network->addConnectionListener( this );
		m_Network->callRpc<myFirstRpc, i32, i32>( 5, 10, true );
		m_Network->createGroup<myFirstVarGroup, string, bool>( "hello bartje k", true, true, 6 );

		auto masterEtp = IEndpoint::resolve( "localhost", 12200 );

		m_Network->registerServer( *masterEtp, "myFirstGame" );
		m_Network->joinServer( *masterEtp, "myFirstGame" );*/

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

	void Game::onConnectResult(INetwork& network, const IEndpoint& etp, EConnectResult res)
	{
		switch (res)
		{
		case MiepMiep::EConnectResult::Succes:
			cout << "connected to: " << etp.toIpAndPort() << endl;
			break;
		case MiepMiep::EConnectResult::Timedout:
			cout << "connecting to: " << etp.toIpAndPort() << " timed out." << endl;
			break;
		case MiepMiep::EConnectResult::InvalidPassword:
			cout << "connecting to: " << etp.toIpAndPort() << " invalid password." << endl;
			break;
		case MiepMiep::EConnectResult::MaxConnectionsReached:
			cout << "connecting to: " << etp.toIpAndPort() << " max connections reached." << endl;
			break;
		case MiepMiep::EConnectResult::AlreadyConnected:
			cout << "connecting to: " << etp.toIpAndPort() << " already connected." << endl;
			break;
		case MiepMiep::EConnectResult::InvalidConnectPacket:
			cout << "connecting to: " << etp.toIpAndPort() << " invalid connection packet." << endl;
			break;
		default:
			cout << "connecting to: " << etp.toIpAndPort() << " unknown result returned!" << endl;
			break;
		}

		
	}

}

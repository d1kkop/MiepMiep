#include "Game.h"
#include "BinSerializer.h"
#include "Apple.h"
#include "Tree.h"
#include <iostream>
#include <tuple>
using namespace std;



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

	template <typename ...Args>
	struct Tuple
	{
		tuple<Args...> tp;

		void init(Args... args, int ab)
		{
			tuple<Args...> _tp(args...);
			tp = _tp;
		}
	};



	Game::Game()
	= default;


	bool Game::init()
	{
	
		m_Network = INetwork::create();

		//m_Network->registerType<Apple>();
		//m_Network->registerType<Tree>();

		tuple<> tp;

		Tuple<> tp2;
		tp2.init(3);
		

		//std::make_tuple(

		m_Network->callRpc<myFirstRpc, i32, i32>( 5, 10, true );

		m_Network->createGroup<myFirstVarGroup, string, bool>( "hello bartje k", true, true, 6 );


	//	m_Network->createType<Apple>();
	//	m_Network->createType<Apple>();

		Apple* apple = new Apple(*m_Network);
		delete apple;

		return true;
	}

	void Game::run()
	{
		auto masterEtp = IEndpoint::resolve( "localhost", 12200 );

		m_Network->registerServer( *masterEtp, "myFirstGame" );
		m_Network->joinServer( *masterEtp, "myFirstGame" );
	}

}

//
//template <> bool MiepMiep::GenericNetVar<i32>::sync( BinSerializer& bs, bool write )
//{
//	return false;
//}
//
//template <> bool MiepMiep::GenericNetVar<u32>::sync( BinSerializer& bs, bool write )
//{
//	return false;
//}
#include <memory>

#pragma once

#include "UnitTestBase.h"
#include "Socket.h"
#include "SocketSet.h"
#include <thread>
#include <mutex>
#include <cassert>
using namespace std;


class Person
{
public:
	Person (i32 race, i32 age, i32 weight):
		m_Race(race),
		m_Age(age),
		m_Weight(weight)
	{
	}


	NetInt32  m_Race;
	NetInt32  m_Age;
	NetUint32 m_Weight;
};

MM_VARGROUP(personVarGroup, i32, i32, u32)
{
	i32 race = get<0>(tp);
	i32 age  = get<1>(tp);
	i32 weight = get<2>(tp);
	new Person( race, age, weight );
}


UTESTBEGIN(VariablesTest)
{
	sptr<INetwork> network = INetwork::create();

	vector<thread> ts;
	for ( i32 i=0; i < 10; i++ )
	{
		ts.emplace_back( [=]() 
		{
			sptr<Person> p = make_shared<Person>( 22, 33, 44 );
			network->createGroup<personVarGroup, i32, i32, i32>( 10, 20, 33 );

			assert( p->m_Age.getGroupId() == -1 );
			assert( p->m_Age.getOwner() == nullptr );
			assert( p->m_Age.getVarConrol() == EVarControl::Full );

			assert( p->m_Weight.getGroupId() == -1 );
			assert( p->m_Weight.getOwner() == nullptr );
			assert( p->m_Weight.getVarConrol() == EVarControl::Full );

			this_thread::sleep_for( std::chrono::milliseconds(20) );
		});
	}

	for ( auto& t : ts )
		t.join();

	return true;
}
UNITTESTEND(VariablesTest)
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
	Person ( INetwork& network ):
		m_Network(network),
		m_Age(22),
		m_Weight(86)
	{
		int k = 0;
	}

	MM_NETWORK_CREATABLE()
	{
		new Person( network );
	}

	NetInt32  m_Race;
	NetInt32  m_Age;
	NetUint32 m_Weight;

	INetwork& m_Network;
};



UTESTBEGIN(VariablesTest)
{
	sptr<INetwork> network = INetwork::create();

	Person* pp = network->createType<Person, INetwork&>( *network );

	sptr<Person> p = sptr<Person>(pp);

	assert( p->m_Age.getGroupId() == -1 );
	assert( p->m_Age.getOwner() == nullptr );
	assert( p->m_Age.getVarConrol() == EVarControl::Unowned );

	assert( p->m_Weight.getGroupId() == -1 );
	assert( p->m_Weight.getOwner() == nullptr );
	assert( p->m_Weight.getVarConrol() == EVarControl::Unowned );

	//assert( p->m_Age.getGroupId() == -1 );
	//assert( p->m_Age.getOwner() == nullptr );
	//assert( p->m_Age.getVarConrol() == EVarControl::Full || p->m_Age.getVarConrol() == EVarControl::Semi );

	//assert( p->m_Weight.getGroupId() == -1 );
	//assert( p->m_Weight.getOwner() == nullptr );
	//assert( p->m_Weight.getVarConrol() == EVarControl::Full || p->m_Weight.getVarConrol() == EVarControl::Semi );

	return true;
}
UNITTESTEND(VariablesTest)
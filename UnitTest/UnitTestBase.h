#pragma once

#include "MiepMiep.h"
#include <string>
#include <vector>
#include <iostream>
using namespace std;
using namespace MiepMiep;


namespace UnitTest
{
	class UnitTestBase
	{
	public:
		UnitTestBase(const char* name): m_Name(name) { }
		virtual bool run() = 0;
		string name() { return m_Name; }
		string m_Name;
	};

	vector<UnitTestBase*> g_UnitTests;
	UnitTestBase* addUTest(UnitTestBase* ut) { g_UnitTests.push_back(ut); return ut; }
}


#define UTESTBEGIN(name) \
namespace UnitTest\
{\
	class UTest##name : public UnitTestBase \
	{\
public:\
		UTest##name(): UnitTestBase(#name) { } \
		bool run() override

#define UNITTESTEND(name)\
	};\
}\
static UnitTest::UTest##name * st_##name = static_cast<UnitTest::UTest##name*>( UnitTest::addUTest( new UnitTest::UTest##name() ) );
#include "UnitTestBase.h"
#include "MyTests.h"
#include "VariableTests.h"
#include "StreamTests.h"
#include "Network.h"
using namespace UnitTest;


int main(int argc, char** arv)
{

	//for ( auto* ut : g_UnitTests )
	//{
	//	if ( ut->run() )
	//	{
	//		cout << ut->name() << " Fine" << endl;
	//	}
	//	else
	//	{
	//		cout << ut->name() << " FAILED" << endl;
	//	}
	//	delete ut;
	//}

	st_ReliableTest->run();


	Network::clearAllStatics();
	Network::printMemoryLeaks();
	system("pause");
}
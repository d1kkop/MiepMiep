#include "UnitTestBase.h"
#include "MyTests.h"
#include "VariableTests.h"
#include "StreamTests.h"
#include "Network.h"
#include "Types.h"
using namespace UnitTest;


int main(int argc, char** arv)
{
	u64 startTime = Util::abs_time();

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

	u64 endTime = Util::abs_time();
	printf("Run time is %llums.\n", (endTime -startTime));
	Network::printMemoryLeaks();
	system("pause");
}
#pragma once

#include "Common.h"
#include <vector>
using namespace std;


namespace MiepMiep
{
	class BinSerializer;
	class NetVariable;


	class PerThreadDataProvider
	{
	public:
		MM_TS static BinSerializer& getSerializer(bool reset=true);
		MM_TS static vector<class NetVariable*>& getConstructedVariables();
	};
}

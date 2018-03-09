#pragma once

#include "BinSerializer.h"
#include "Common.h"
#include <thread>
#include <map>
#include <vector>
using namespace std;


namespace MiepMiep
{
	struct PerThreadData
	{
		BinSerializer m_Serializer;
		vector<class NetVariable*> m_ConstructedVariables;
	};


	class PerThreadDataProvider
	{
	public:
		MM_TS static BinSerializer& getSerializer(bool reset=true);
		MM_TS static BinSerializer& beginSend();
		MM_TS static vector<class NetVariable*>& getConstructedVariables();
		MM_TS static void cleanupStatics();

	private:
		static mutex m_PerThreadMapMutex;
		static map<thread::id, PerThreadData> m_PerThreadDataMap;
	};
}

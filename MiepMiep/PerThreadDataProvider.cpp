#include "PerThreadDataProvider.h"
#include <cassert>


namespace MiepMiep
{
	MM_TS BinSerializer& PerThreadDataProvider::getSerializer(bool reset)
	{
		scoped_lock lk(m_PerThreadMapMutex);
		auto& bs = m_PerThreadDataMap[ this_thread::get_id() ].m_Serializer;
		if ( reset ) bs.reset();
		return bs;
	}

	MM_TS vector<NetVariable*>& PerThreadDataProvider::getConstructedVariables()
	{
		scoped_lock lk(m_PerThreadMapMutex);
		return m_PerThreadDataMap[ this_thread::get_id() ].m_ConstructedVariables;
	}

	MM_TS void PerThreadDataProvider::cleanupStatics()
	{
		scoped_lock lk(m_PerThreadMapMutex);
		m_PerThreadDataMap.clear();
	}

	MM_TS BinSerializer& priv_get_thread_serializer()
	{
		return PerThreadDataProvider::getSerializer();
	}

	// Linking
	mutex PerThreadDataProvider::m_PerThreadMapMutex;
	map<thread::id, PerThreadData> PerThreadDataProvider::m_PerThreadDataMap;
}




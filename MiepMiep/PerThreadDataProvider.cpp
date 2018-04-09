#include "PerThreadDataProvider.h"
#include "BinSerializer.h"


namespace MiepMiep
{
	MM_TS BinSerializer& PerThreadDataProvider::getSerializer(bool reset)
	{
		static thread_local BinSerializer tl_binSerializers;
		if ( reset ) tl_binSerializers.reset();
		return tl_binSerializers;
	}

	MM_TS vector<NetVariable*>& PerThreadDataProvider::getConstructedVariables()
	{
		static thread_local vector<NetVariable*> tl_constructionVariables;
		return tl_constructionVariables;
	}

	MM_TS BinSerializer& priv_get_thread_serializer()
	{
		return PerThreadDataProvider::getSerializer();
	}
}




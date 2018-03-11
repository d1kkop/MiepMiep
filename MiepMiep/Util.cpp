#include "Util.h"
#include "Platform.h"
using namespace chrono;

namespace MiepMiep
{
	u32 Util::htonl(u32 val)
	{
	#if MM_LIL_ENDIAN
		return ((val & 255)<<24) | (val>>24) | ((val & 0xFF0000)>>8) | ((val & 0xFF00)<<8);
	#else
		return val;
	#endif
	}

	u16 Util::htons(u16 val)
	{
	#if MM_LIL_ENDIAN
		return ((val & 255)<<8) | (val>>8);
	#else
		return val;
	#endif
	}

	u64 Util::abs_time()
	{
		high_resolution_clock::time_point cl;
		return duration_cast<milliseconds>( cl.time_since_epoch() ).count();
	}

}
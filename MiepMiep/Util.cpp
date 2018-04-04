#include "Util.h"
#include "Platform.h"
#include "Threading.h"
using namespace chrono;


namespace MiepMiep
{

	u64 Util::htonll(u64 x)
	{
	#if MM_LIL_ENDIAN
		x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
		x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
		x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
		return x;
	#else
		return x;
	#endif
	}

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
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

	u32 Util::rand()
	{
		static SpinLock sl;
		scoped_spinlock lk(sl);
		return ::rand();
	}

}
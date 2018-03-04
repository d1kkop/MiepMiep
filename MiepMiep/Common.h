#pragma once


#include "Config.h"
#include "Types.h"
#include <cassert>
#include <mutex>
using namespace std;


#define MM_FUNCTION __FUNCTION__
#define MM_LINE		__LINE__
#define MM_FL		MM_FUNCTION, MM_LINE
#define MM_VARARGS	__VA_ARGS__

#define __CHECKED( expr ) if ( !(expr) ) { assert(false); return; }
#define __CHECKEDB( expr ) if ( !(expr) ) { assert(false); return false; }


namespace MiepMiep
{
	using scoped_lock	= lock_guard<mutex>;
	using rscoped_lock	= lock_guard<recursive_mutex>;
	using rmutex = recursive_mutex;
	using rmutex = std::recursive_mutex;
	template <typename T>	using wptr = weak_ptr<T>;
	template <typename T>	using uptr = unique_ptr<T>;
}
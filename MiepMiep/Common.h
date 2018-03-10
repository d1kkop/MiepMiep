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
#define MM_RPC_CHANNEL 0
#define MM_VG_CHANNEL  0
#define MM_SOCK_SELECT_TIMEOUT 100

/* Serialization properties */
#define MM_MAX_FRAGMENTSIZE 1900
#define MM_MAX_RECVSIZE 4096
#define MM_MIN_HDR_SIZE 5
#define MM_CHANNEL_MASK 7
#define MM_RELAY_BIT 4
#define MM_FRAGMENT_FIRST_BIT 8
#define MM_FRAGMENT_LAST_BIT 16
#define MM_SYSTEM_BIT 32

#define __CHECKED( expr ) if ( !(expr) ) { assert(false); LOGC("Serialization error!"); return; }
#define __CHECKEDB( expr ) if ( !(expr) ) { assert(false); LOGC("Serialization error!"); return false; }
#define __CHECKEDSR( expr ) if ( !(expr) ) { assert(false); LOGC("Serialization error!"); return ESendCallResult::SerializationError; }


#define MM_TO_PTR( TYPE ) \
	sptr<TYPE> to_ptr() { return ptr<TYPE>(); } \
	sptr<const TYPE> to_ptr() const { return ptr<const TYPE>(); }


namespace MiepMiep
{
	using scoped_lock	= lock_guard<mutex>;
	using rscoped_lock	= lock_guard<recursive_mutex>;
	using rmutex = recursive_mutex;
	using rmutex = std::recursive_mutex;
	template <typename T>	using wptr = weak_ptr<T>;
	template <typename T>	using uptr = unique_ptr<T>;
}
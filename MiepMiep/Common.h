#pragma once


#include "Config.h"
#include "Types.h"
#include <cassert>
#include <mutex>
using namespace std;


#define MM_FUNCTION __FUNCTION__
#define MM_LINE		__LINE__
#define MM_FL		MM_FUNCTION, MM_LINE
#define MM_RPC_CHANNEL 0
#define MM_VG_CHANNEL  0
#define MM_SOCK_SELECT_TIMEOUT 100
#define MM_NEWER_SEQ_RANGE (UINT_MAX>>2)


/* Serialization properties */
#define MM_MAX_FRAGMENTSIZE 1900
#define MM_MAX_RECVSIZE 4096
#define MM_MAX_SENDSIZE MM_MAX_RECVSIZE
#define MM_MIN_HDR_SIZE 9 /* linkId(4) + seq(4) + compId(1) */
#define MM_FRAGMENT_HDR_SIZE 6 /* compId(1) + channAndFlags(1) + dataId(4) */
#define MM_CHANNEL_MASK 7
#define MM_RELAY_BIT 4
#define MM_FRAGMENT_FIRST_BIT 8
#define MM_FRAGMENT_LAST_BIT 16
#define MM_SYSTEM_BIT 32

/* (Re)send thread */
#define MM_ST_LINKS_CLUSTER_SIZE 64
#define MM_ST_RESEND_CHECK_INTERVAL 4 /* ms */

/* Congestion control & stats */
#define MM_MIN_RESEND_LATENCY_MP 1.3f
#define MM_MAX_RESEND_LATENCY_MP 10.f

/*	At what number create new server list */
#define MM_NEW_SERVER_LIST_THRESHOLD 1000

#define __CHECKED( expr ) if ( !(expr) ) { assert(false); LOGC("Serialization error!"); return; }
#define __CHECKEDB( expr ) if ( !(expr) ) { assert(false); LOGC("Serialization error!"); return false; }
#define __CHECKEDSR( expr ) if ( !(expr) ) { assert(false); LOGC("Serialization error!"); return ESendCallResult::SerializationError; }


#define MM_TO_PTR( TYPE ) \
	sptr<TYPE> to_ptr(); \
	sptr<const TYPE> to_ptr() const;

#define MM_TO_PTR_IMP( TYPE) \
	sptr<TYPE> TYPE::to_ptr() { return ptr<TYPE>(); } \
	sptr<const TYPE> TYPE::to_ptr() const { return ptr<const TYPE>(); }

#define RPC_BEGIN() \
	assert(link); \
	if (!link) { LOGW("Unexpectd local RPC call."); return; } \
	auto& nw = toNetwork(network); \
	auto& l  = sc<Link&>( const_cast<ILink&>(*link) )


namespace MiepMiep
{
	using scoped_lock   = lock_guard<mutex>;
	using rscoped_lock  = lock_guard<recursive_mutex>;
	using rmutex		= recursive_mutex;

	template <typename T> using wptr = weak_ptr<T>;
	template <typename T> using uptr = unique_ptr<T>;

	template <typename T, typename F>
	inline T rc (F&& f) { return reinterpret_cast<T>(f); }
	template <typename T, typename F>
	inline T sc (F&& f) { return static_cast<T>(f); }
	template <typename T, typename F>
	inline T scc (F&& f) { return static_cast<T>(const_cast<T>(f)); }
}
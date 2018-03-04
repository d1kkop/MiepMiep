#pragma once

#include "Types.h"
#include "BinSerializer.h"
using namespace MiepMiep;


namespace MiepMiep
{
	template <size_t N, typename TP>
	void serialize(BinSerializer& bs, bool write, TP& tp) { }
	template <size_t N, typename TP, typename T, typename ...Args>
	void serialize(BinSerializer& bs, bool write, TP& tp)
	{
		T& te = std::get<N>(tp);
		bs.readOrWrite<T> ( te, write );
		serialize<N+1, TP, Args...>( bs, write, tp );
	}
}

#define MM_CALL_RPC_LOCAL

#define MM_RPC(name, ...) \
inline void rpc_tuple_##name( INetwork& network, const IEndpoint* etp, const std::tuple<__VA_ARGS__>& tp ); \
struct name { \
template <typename ...Args> \
inline static void rpc( Args... args, INetwork& network, BinSerializer& bs, bool localCall )\
{ \
	using va_tuple = std::tuple<Args...>; \
	va_tuple tp(args...); \
	MiepMiep::serialize<0, va_tuple, __VA_ARGS__>( bs, true, tp ); \
	if ( localCall ) rpc_tuple_##name( network, nullptr, tp ); \
}};\
extern "C" {\
inline void MM_DLL_EXPORT deserialize_##name(INetwork& network, const IEndpoint* etp, BinSerializer& bs)\
{ \
	using va_tuple = std::tuple<__VA_ARGS__>; \
	va_tuple tp; \
	MiepMiep::serialize<0, va_tuple, __VA_ARGS__>( bs, false, tp ); \
	rpc_tuple_##name( network, etp, tp ); \
} \
}\
void rpc_tuple_##name(INetwork& network, const IEndpoint* etp, const std::tuple<__VA_ARGS__>& tp)

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
		readOrWrite<T> ( bs, te, write );
		serialize<N+1, TP, Args...>( bs, write, tp );
	}
}


#define MM_RPC(name, ...) \
inline void rpc_tuple_##name( INetwork& network, const ILink* link, const std::tuple<__VA_ARGS__>& tp ); \
struct name { \
inline static const char* rpcName() { return #name; } \
template <typename ...Args> \
inline static void rpc(Args... args, INetwork& network, BinSerializer& bs, bool localCall )\
{ \
	std::tuple<Args...> tp(args...); \
	MiepMiep::serialize<0, std::tuple<Args...>, Args...>( bs, true, tp ); \
	if ( localCall ) rpc_tuple_##name( network, nullptr, tp ); \
}\
template <typename ...Args> \
inline static void rpc( INetwork& network, BinSerializer& bs, bool localCall ) /* zero params case */\
{\
	if ( localCall ) rpc_tuple_##name( network, nullptr, std::tuple<>() ); \
}};\
extern "C" {\
inline void MM_DLL_EXPORT rpc_dsr_##name(INetwork& network, const ILink& link, BinSerializer& bs)\
{ \
	std::tuple<__VA_ARGS__> tp; \
	MiepMiep::serialize<0, std::tuple<__VA_ARGS__>, __VA_ARGS__>( bs, false, tp ); \
	rpc_tuple_##name( network, &link, tp ); \
} \
}\
void rpc_tuple_##name(INetwork& network, const ILink* link, const std::tuple<__VA_ARGS__>& tp)


#define MM_VARGROUP(name, ...) \
	MM_RPC(name, __VA_ARGS__)
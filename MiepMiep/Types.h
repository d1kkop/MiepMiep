#pragma once


#if !(MM_EXPORTING || MM_IMPORTING)
	#define MM_DECLSPEC
#else
	#if MM_EXPORTING
		#define MM_DECLSPEC __declspec(dllexport)
	#else
		#define MM_DECLSPEC __declspec(dllimport)
	#endif
#endif


#define MM_COUNTER __COUNTER__
#define MM_DLL_EXPORT __declspec(dllexport)


#define MM_TS  /* Thread safe function. */


#include <map>
#include <memory>



#define MM_NETWORK_CREATABLE() \
	static u16 typeId() { return MM_COUNTER; } \
	static void remoteCreate(INetwork& network, const IEndpoint& etp)


namespace MiepMiep
{
	MM_TS MM_DECLSPEC extern class BinSerializer& get_thread_serializer();
	MM_TS MM_DECLSPEC extern void* get_rpc_func(const std::string& name);
}


namespace MiepMiep
{
	class BinSerializer;
	using byte = unsigned char;
	using i16  = short;
	using i32  = long;
	using i64  = long long;
	using u16  = unsigned short;
	using u32  = unsigned long;
	using u64  = unsigned long long;
	using MetaData = std::map<std::string, std::string>;
	template <typename T> using sptr = std::shared_ptr<T>;
}
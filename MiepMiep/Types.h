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
#define MM_ASYNC


#include <map>
#include <memory>


namespace MiepMiep
{
	using byte = unsigned char;
	using i16  = short;
	using i32  = long;
	using i64  = long long;
	using u16  = unsigned short;
	using u32  = unsigned long;
	using u64  = unsigned long long;

	class IAddress;
	class ILink;
	class INetwork;
	class BinSerializer;
	class IDeliveryTrace;
	class IConnectionListener;

	enum class EPacketType : byte;
	enum class EConnectResult : byte;
	enum class EDisconnectReason : byte;

	enum class EVarControl;
	enum class ESendResult;
	enum class EChangeOwnerCallResult;
	enum class ESendCallResult;
	enum class EListenCallResult;
	enum class EJoinServerCallResult;
	enum class ERegisterServerCallResult;

	using MetaData = std::map<std::string, std::string>;
	template <typename T> using sptr = std::shared_ptr<T>;

	struct IEndpoint_less
	{
		bool operator()( const IAddress& left, const IAddress& right ) const;
		bool operator()( const sptr<const IAddress>& left, const sptr<const IAddress>& right ) const;
	};


	// ---- !! FOR INTERNAL USE ONLY !! ------
	MM_TS MM_DECLSPEC extern BinSerializer& priv_get_thread_serializer();
	MM_TS MM_DECLSPEC extern ESendCallResult priv_send_rpc(INetwork& nw, const char* rpcName, BinSerializer& bs, const IAddress* specific, bool exclude, bool buffer, bool relay, bool sysBit, byte channel, IDeliveryTrace* trace); 
	MM_TS MM_DECLSPEC extern void  priv_create_group(INetwork& nw, const char* groupType, BinSerializer& bs, byte channel, IDeliveryTrace* trace);
	MM_TS MM_DECLSPEC extern void* priv_get_rpc_func(const std::string& name);
}
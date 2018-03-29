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


#define MM_TS	/* Thread safe function. */
#define MM_TSC	/* Thread safe in a sense that data from this function is only ever initialized from constructor. No locks are required to read this data. */


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
	class ISession;
	class INetwork;
	class BinSerializer;
	class IDeliveryTrace;
	class ISessionListener;

	enum class EPacketType : byte;
	enum class EConnectResult : byte;
	enum class EDisconnectReason : byte;
	enum class ERegisterServerResult : byte;
	enum class EJoinServerResult : byte;

	enum class EVarControl;
	enum class ESendResult;
	enum class EChangeOwnerCallResult;
	enum class ESendCallResult;
	enum class EListenCallResult;
	enum class ECreateGroupCallResult;


	template <typename T>
	using sptr		= std::shared_ptr<T>;
	using MetaData	= std::map<std::string, std::string>;


	// For clarity instead of a range of  true/false params which make little sense when reading back.

	constexpr bool No_Local  = false;
	constexpr bool No_Relay  = false;
	constexpr bool No_Buffer = false;
	constexpr bool No_SysBit = false;

	constexpr bool Do_Local  = true;
	constexpr bool Do_Relay  = true;
	constexpr bool Do_Buffer = true;
	constexpr bool Do_SysBit = true;

	constexpr auto No_Trace = nullptr;



	// ---- !! FOR INTERNAL USE ONLY !! ------
	MM_TS MM_DECLSPEC extern BinSerializer& priv_get_thread_serializer();
	MM_TS MM_DECLSPEC extern ESendCallResult priv_send_rpc(INetwork& nw, const char* rpcName, BinSerializer& bs, const ISession* session, ILink* exclOrSpecific, bool buffer, bool relay, bool sysBit, byte channel, IDeliveryTrace* trace); 
	MM_TS MM_DECLSPEC extern ECreateGroupCallResult priv_create_group(INetwork& nw, const ISession& session, const char* groupType, BinSerializer& bs, byte channel, IDeliveryTrace* trace);
	MM_TS MM_DECLSPEC extern void* priv_get_rpc_func(const std::string& name);
}
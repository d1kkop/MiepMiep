#pragma once


#include "Common.h"


#define MM_LIL_ENDIAN					(1)
#define MM_BIG_ENDIAN					(0)



#if MM_PLATFORM_WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <ws2tcpip.h>
	#include <ws2ipdef.h>
	#include <windows.h>
	#undef min
	#undef max
	#undef DELETE
	#pragma comment(lib, "Ws2_32.lib")
	#pragma comment(lib, "User32.lib")
#endif


namespace MiepMiep
{
	enum class ELogType
	{
		Message,
		Warning, 
		Critical
	};


	class Platform
	{
	public:
		/*	Initializes if refcount is 0, otherwise only increments refcount. 
			0 is returned on succes. Socket error code otherwise.
			Additional calls result in returning 0 if the first call was succesful. */
		MM_TS static u32 initialize();

		/*	Returns false if did not 'really' close. That is, it only reduced the reference count.
			Only when refcount becomes zero, it actually shuts down. */
		MM_TS static bool shutdown();

		// Obtain ptr to address in executing img, given that the function was exported.
		MM_TS static void* getPtrFromName(const char* name);

		// Log
		MM_TS static void log( ELogType ltype, const char* fname, u64 line, const char* msg, ... );

		// Returns local time as string
		MM_TS static string localTime();

		static void setLogSettings(bool logToFile, bool logToIde);

		// Rentrent
		static void vsprintf(char* buff, u64 size, const char* msg, va_list& vaList);
		static void formatPrint(char* dst, u64 dstSize, const char* frmt, ... );
		static void sleep( u32 milliSeconds );
		static void memCpy(void* dst, u64 size, const void* src, u64 srcSize);
		static void fprintf(FILE* f, const char* msg);
		template<typename T> static void copy(T* dst, const T* src, u64 cnt);

	private:
		static void logToFile( const char* msg );
		static void logToIde( const char* msg );

		static u32   m_NumInitialized;
		static bool  m_LogToFile;
		static bool  m_LogToIde;
		static mutex m_InitMutex;
		static mutex m_LocalTimeMutex;
		static mutex m_Name2FuncMutex;
		static recursive_mutex m_LogMutex;
		static map<string, void*> m_Name2Func;
	};


	template<typename T>
	void Platform::copy(T* dst, const T* src, u64 cnt)
	{
		memCpy(dst, sizeof(T)*cnt, src, sizeof(T)*cnt);
	}

}

//#if MM_DEBUG
#define LOG( msg, ... )  Platform::log( ELogType::Message, MM_FUNCTION, MM_LINE, msg, __VA_ARGS__ )
//#else
//#define LOG( msg, ... )
//#endif 

#define LOGW( msg, ... ) assert(false); Platform::log( ELogType::Warning, MM_FUNCTION, MM_LINE, msg, __VA_ARGS__ )
#define LOGC( msg, ... ) assert(false); Platform::log( ELogType::Critical, MM_FUNCTION, MM_LINE, msg, __VA_ARGS__ )

	
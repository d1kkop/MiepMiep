#pragma once


#if _DEBUG
#define MM_DEBUG										(1)
#endif

//#if MM_DEBUG
	#define MM_TRACE_MEMORY_LEAKS						(0)
	#define MM_USE_CALLOC								(1)
//#endif

#define MM_TRACE_JOBSYSTEM								(0)
#define MM_PRINT_ALLOCATIONS							(0)
#define MM_SECURE_CRT									(0)
#define MM_INCLUDE_WINHDR								(1)

#define MM_PLATFORM_WINDOWS								(1)

#if MM_PLATFORM_WINDOWS
	#define MM_WIN32SOCKET								(1)
	#define MM_SDLSOCKET								(0)
	#define MM_SDLCORE									(0)
	
	#if !MM_SECURE_CRT
		#define _CRT_SECURE_NO_WARNINGS
	#endif

#endif


#if !(MM_EXPORTING || MM_IMPORTING)
	#define MM_DECLSPEC_INTERN
#else
	#if MM_EXPORTING
		#define MM_DECLSPEC_INTERN __declspec(dllexport)
	#else
		#define MM_DECLSPEC_INTERN __declspec(dllimport)
	#endif
#endif

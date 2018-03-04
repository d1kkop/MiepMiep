#include "Platform.h"
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <cstdarg>
#include <string>
using namespace std::chrono;


namespace MiepMiep
{
	u32 Platform::initialize()
	{
		scoped_lock lk(m_InitMutex);

		if ( m_NumInitialized++ != 0 )
		{
			return 0;
		}

		srand ((u32)time(nullptr));

	#if MM_SDLSOCKET
		u32 err = (u32) SDL_Init(0);
		if (err != 0) 
		{ 
			Platform::log("SDL error %s.", SDL_GetError());
			return err; 
		}
		err = SDLNet_Init();
		if (err != 0) 
		{
			Platform::log("SDL net error %s.", SDL_GetError());
			return err;
		}
	#elif MM_WIN32SOCKET
		WORD wVersionRequested;
		WSADATA wsaData;
		u32 err;

		/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
		wVersionRequested = MAKEWORD(2, 2);

		err = WSAStartup(wVersionRequested, &wsaData);
		if ( err != 0 )
			return err;

		/* Confirm that the WinSock DLL supports 2.2.*/
		/* Note that if the DLL supports versions greater    */
		/* than 2.2 in addition to 2.2, it will still return */
		/* 2.2 in wVersion since that is the version we      */
		/* requested.                                        */

		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
		{
			/* Tell the user that we could not find a usable */
			/* WinSock DLL.                                  */
			WSACleanup();
			return -1;
		}
	#endif

		return 0;
	}

	bool Platform::shutdown()
	{
		scoped_lock lk(m_InitMutex);

		if ( m_NumInitialized == 0 )  return false;
		if ( --m_NumInitialized > 0 ) return false;

	#if MM_SLDSOCKET
		SDLNet_Quit();
		SDL_Quit();
	#else
		#if MM_WIN32SOCKET
			WSACleanup();
		#endif
	#endif

		// actually shutdown
		return true;
	}

	void* Platform::getPtrFromName(const char* name)
	{
		scoped_lock lk(m_Name2FuncMutex);

		auto it = m_Name2Func.find( name );
		if ( it == m_Name2Func.end() )
		{
			// ptr to function
			void* pf = nullptr;

		#if MM_SDLCORE
			pf = SDL_LoadFunction(nullptr, name);
		#else
			#if MM_PLATFORM_WINDOWS
				HMODULE hModule = ::GetModuleHandle(nullptr);
				pf = ::GetProcAddress( hModule, name );
			#endif
		#endif

			if ( pf )
			{
				m_Name2Func.insert( std::make_pair( name, pf ) );
			}

			return pf;
		}
		return it->second;
	}

	// To get from user code to find func -->
	MM_TS void* get_rpc_func(const std::string& name)
	{
		return Platform::getPtrFromName( (std::string("rpc_") + name).c_str() );
	}

	void Platform::log(ELogType ltype, const char* fname, u64 line, const char* msg, ...)
	{
		rscoped_lock lk(m_LogMutex);

		char buff[8192];
		va_list myargs;
		va_start(myargs, msg);
		vsprintf_s(buff, 8190, msg, myargs);
		va_end(myargs);

		auto timeStr = localTime();
		string finalStr;

		if ( ltype != ELogType::Message )
		{
			string warnStr;
			switch (ltype)
			{
			case ELogType::Warning:
				warnStr = " WARNING: ";
				break;
			case ELogType::Critical:
				warnStr = " CRITICAL ";
				break;
			}
			finalStr = timeStr + warnStr + std::string(fname) + " line: " + to_string(line) + " " + string(buff) + "\n";
		}
		else
		{
			finalStr = timeStr + " " + string(buff) + "\n";
		}

		// On first open attach new session in front.
		static bool isFirstOpen = true;
		if (isFirstOpen)
		{
			isFirstOpen = false;
			log(ELogType::Message, MM_FUNCTION, MM_LINE, "------- New Session --------");
			log(ELogType::Message, MM_FUNCTION, MM_LINE, "----------------------------");
		}
		 
		logToFile( finalStr.c_str() );
		logToIde( finalStr.c_str() );
	}

	void Platform::logToFile(const char* msg)
	{
		if (!m_LogToFile) return;

		static bool isFirstOpen = true;
		static const char* fileName = "MiepMiep.txt";

		// Delete old
		if (isFirstOpen) ::remove(fileName);
		isFirstOpen = false;

		// (Re)open file
		FILE* f;
	#if MM_SECURE_CRT
		fopen_s(&f, fileName, "a");
	#else
		f = fopen(fileName, "a");
	#endif

		if (!f) return;

		fprintf(f, msg);
		fclose(f);
	}

	void Platform::logToIde(const char* msg)
	{
	#if MM_INCLUDE_WINHDR
		::OutputDebugString(msg);
	#endif
	}

	void Platform::formatPrint(char* dst, u64 dstSize, const char* fmt, ...)
	{
		va_list myargs;
		va_start(myargs, fmt);
		vsprintf(dst, dstSize, fmt, myargs);
		va_end(myargs);
	}

	void Platform::sleep(u32 ms)
	{
		this_thread::sleep_for(milliseconds(ms));
	}

	void Platform::memCpy(void* dst, u64 size, const void* src, u64 count)
	{
	#if MM_SECURE_CRT
		memcpy_s(dst, size, src, count);
	#else
		memcpy(dst, src, count);
	#endif
	}

	void Platform::vsprintf(char* buff, u64 size, const char* msg, va_list& vaList)
	{
		assert(buff && msg);
	#if MM_SECURE_CRT
		vsprintf_s(buff, size, msg, vaList);
	#else
		::vsprintf(buff, msg, vaList);
	#endif
	}

	void Platform::fprintf(FILE* f, const char* msg)
	{
		assert(f && msg);
	#if MM_SECURE_CRT
		fprintf_s(f, "%s", msg);
	#else
		::fprintf(f, msg);
	#endif
	}

	string Platform::localTime()
	{
		scoped_lock lk(m_LocalTimeMutex);

		time_t rawtime;
		struct tm timeinfo;
		time (&rawtime);
		localtime_s(&timeinfo, &rawtime);
		char asciitime[256];
		asctime_s(asciitime, 256, &timeinfo);
		// For some reason it attaches a new line..
		char* p = strstr(asciitime, "\n");
		if ( p )
		{
			*p ='\0';
			return asciitime;
		}
		return "";
	}


	void Platform::setLogSettings(bool logToFile, bool logToIde)
	{
		m_LogToFile = logToFile;
		m_LogToIde  = logToIde;
	}

	// Linking
	u32 Platform::m_NumInitialized = 0;
	bool Platform::m_LogToFile = true;
	bool Platform::m_LogToIde  = true;
	mutex Platform::m_InitMutex;
	mutex Platform::m_Name2FuncMutex;
	mutex Platform::m_LocalTimeMutex;
	recursive_mutex Platform::m_LogMutex;
	map<string, void*> Platform::m_Name2Func;
}

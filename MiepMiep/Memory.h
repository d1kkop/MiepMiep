#pragma once

#include "Common.h"
#include "Config.h"
using namespace std;


namespace MiepMiep
{
	struct MemoryFootprint
	{
		string func;
		u64 line;
		u64 size;
	};


	class ITraceable
	{
	public:
		virtual ~ITraceable();
	};


	class Memory
	{
	public:
		MM_TS template <typename T> static T* doAlloc(char* fname, u64 line);
		MM_TS template <typename T, typename... Args> static T* doAlloc(char* fname, u64 line, Args... args);
		MM_TS template <typename T> static T* doAllocN(char* fname, u64 line, u64 cnt);
		MM_TS template <typename T> static void doDelete(T* p);
		MM_TS template <typename T> static void doDeleteN(T* p);

		MM_TS static void printUntracedMemory();

	private:
		MM_TS static void* trace(void* p, u64 size, char* fname, u64 line);
		MM_TS static ITraceable* trace(ITraceable* p, u64 size, char* fname, u64 line);
		MM_TS static void untrace(void* p);
		MM_TS static void untrace(ITraceable* p);

	private:
		static mutex m_MemoryMutex;
		static mutex m_MemoryMutexRaw;
		static map<ITraceable*, MemoryFootprint> m_Memory;
		static map<void*, MemoryFootprint> m_MemoryRaw;

		friend class ITraceable;
	};

	template <typename T>
	T* Memory::doAlloc(char* fname, u64 line)
	{
		return (T*)trace(new T, sizeof(T), fname, line);
	}

	template <typename T, typename... Args>
	T* Memory::doAlloc(char* fname, u64 line, Args... args)
	{
		return (T*)trace(new T(args...), sizeof(T), fname, line);
	}

	template <typename T>
	T* Memory::doAllocN(char* fname, u64 line, u64 cnt)
	{
		return (T*)trace(new T[cnt], sizeof(T)*cnt, fname, line);
	}

	template <typename T>
	void Memory::doDelete(T* p)
	{
		if (!p) return;
		untrace(p);
		delete p;
	}

	template <typename T>
	void Memory::doDeleteN(T* p)
	{
		if (!p) return;
		untrace(p);
		delete [] p;
	}


	// Inline functions to avoid having to type Memory:: in front everywhere.
	template <typename T>						inline T*	reserve(char* fname, u64 line)							{ return Memory::doAlloc<T>(fname, line); }
	template <typename T>						inline T*	reserveN(char* fname, u64 line, u64 cnt)				{ return Memory::doAllocN<T>(fname, line, cnt); }
	template <typename T, typename ...Args>		inline T*	reserve(char* fname, u64 line, Args... args)			{ return Memory::doAlloc<T, Args...>(fname, line, args...); }

	template <typename T>	inline void release(T* p)		{ Memory::doDelete<T>(p); }
	template <typename T>	inline void releaseN(T* p)		{ Memory::doDeleteN<T>(p); }
}
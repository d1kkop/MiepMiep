#include "Memory.h"
#include "Platform.h"
#include <cassert>


void* operator new (size_t n)					
{
	return calloc(1, n); 
}
void* operator new [] (size_t n)				
{ 
	return calloc(1, n); 
}


namespace MiepMiep
{
	ITraceable::~ITraceable()
	{
		Memory::untrace(this);
	}

	void Memory::printUntracedMemory()
	{
	#if MM_TRACE_MEMORY_LEAKS
		scoped_lock lk(m_MemoryMutex);
		for ( auto& kvp : m_Memory )
		{
			MemoryFootprint& mf = kvp.second;
			Platform::log(ELogType::Message, "", 0, "Leak at: %s, line %d, size %d.", mf.func.c_str(), mf.line, mf.size);
		}
		scoped_lock lkRaw(m_MemoryMutexRaw);
		for ( auto& kvp : m_MemoryRaw )
		{
			MemoryFootprint& mf = kvp.second;
			Platform::log(ELogType::Message, "", 0, "Leak at: %s, line %d, size %d.", mf.func.c_str(), mf.line, mf.size);
		}
		if ( m_MemoryRaw.empty() && m_Memory.empty() )
		{
			Platform::log(ELogType::Message, "", 0, "No memory leaks detected.");
		}
	#endif
	}

	ITraceable* Memory::trace(ITraceable* p, u64 size, char* fname, u64 line)
	{
	#if MM_TRACE_MEMORY_LEAKS
		MemoryFootprint mf;
		mf.func = fname;
		mf.line = line;
		mf.size = size;
		scoped_lock lk(m_MemoryMutex);
		assert( m_Memory.count(p) == 0 );
		m_Memory[p] = mf;
	#endif
	#if MM_PRINT_ALLOCATIONS
		Platform::log(ELogType::Message, MM_FL, "Allocating: %s, line %d, size %d.", fname, line, size);
	#endif
		return p;
	}

	void* Memory::trace(void* p, u64 size, char* fname, u64 line)
	{
	#if MM_TRACE_MEMORY_LEAKS
		MemoryFootprint mf;
		mf.func = fname;
		mf.line = line;
		mf.size = size;
		scoped_lock lk(m_MemoryMutexRaw);
		assert( m_MemoryRaw.count(p) == 0 );
		m_MemoryRaw[p] = mf;
	#endif
	#if MM_PRINT_ALLOCATIONS
		Platform::log(ELogType::Message, MM_FL, "Allocating: %s, line %d, size %d.", fname, line, size);
	#endif
		return p;
	}

	void Memory::untrace(ITraceable* p)
	{
	#if MM_TRACE_MEMORY_LEAKS
		scoped_lock lk(m_MemoryMutex);
	//  TODO check for better tracking solution
	//  If put on stack or created with 'new', p is not tracked.
	//	assert( m_Memory.count(p) == 1 );
		m_Memory.erase(p);
	#endif
	}

	void Memory::untrace(void* p)
	{
	#if MM_TRACE_MEMORY_LEAKS
		scoped_lock lk(m_MemoryMutexRaw);
	//  TODO check for better tracking solution
	//  If put on stack or created with 'new', p is not tracked.
	//	assert( m_MemoryRaw.count(p) == 1 );
		m_MemoryRaw.erase(p);
	#endif
	}

	// Linking
	mutex Memory::m_MemoryMutex;
	mutex Memory::m_MemoryMutexRaw;
	map<ITraceable*, MemoryFootprint> Memory::m_Memory;
	map<void*, MemoryFootprint> Memory::m_MemoryRaw;

}
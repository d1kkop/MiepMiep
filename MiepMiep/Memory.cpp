#include "Memory.h"
#include "Platform.h"
#include <cassert>



void* operator new (size_t n)
{
	assert( n <= 1024*1024 );
#if MM_USE_CALLOC
	return calloc(1, n);
#else
	return malloc(n);
#endif
}
void* operator new [] (size_t n)
{ 
#if MM_USE_CALLOC
	return calloc(1, n);
#else
	return malloc(n);
#endif
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
			Platform::log(ELogType::Message, "", 0, "!!** Leak at -->: %s, line %d, size %d. **!!", mf.func.c_str(), mf.line, mf.size);
		}
		if ( m_Memory.empty() )
		{
			Platform::log(ELogType::Message, "", 0, "<< NO MEMORY leaks detected >>.");
		}
	#endif
	}

	ITraceable* Memory::trace(ITraceable* p, u64 size, char* fname, u64 line)
	{
		p->m_Ptr = Shared<ITraceable>( p );
		trace( sc<void*>( p ), size, fname, line );
		return p;
	}

	void* Memory::trace(void* p, u64 size, char* fname, u64 line)
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
		Platform::log(ELogType::Message, MM_FL, "Allocating: %p, %s, line %d, size %d.", p, fname, line, size);
	#endif
		return p;
	}

	void Memory::untrace(ITraceable* p)
	{
		bool removed = untrace( sc<void*>( p ) );
	//	assert( removed ); For stack allocated managed objects, this does not holds.
	}

	bool Memory::untrace(void* p)
	{
	#if MM_PRINT_ALLOCATIONS
		Platform::log(ELogType::Message, MM_FL, "Deallocating: %p", p);
	#endif
	#if MM_TRACE_MEMORY_LEAKS
		scoped_lock lk(m_MemoryMutex);
		auto it = m_Memory.find( p );
		if ( it != m_Memory.end() )
		{
			m_Memory.erase(it);
			return true;
		}
	#endif
		return false;
	}

	// Linking
	#if MM_TRACE_MEMORY_LEAKS
		mutex Memory::m_MemoryMutex;
		map<void*, MemoryFootprint> Memory::m_Memory;
	#endif

}
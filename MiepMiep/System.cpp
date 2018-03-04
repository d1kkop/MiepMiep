#pragma once

#include "Memory.h"
#include "System.h"
#include "Platform.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{
	// -------- System -------------------------------------------------------------------

	void SystemIntern::clearStatics()
	{
		PerThreadDataProvider::cleanupStatics();
	}

	void SystemIntern::printMemoryLeaks()
	{
		Memory::printUntracedMemory();
	}

	// -------- System (that user can acces). -------------------------------------------------------------------

	bool System::open()
	{
		return true;
	}

	void System::close()
	{
		SystemIntern::clearStatics();
		SystemIntern::printMemoryLeaks();
	}

	void System::setLogSettings(bool logToFile, bool logToIde)
	{
		Platform::setLogSettings( logToFile, logToIde );
	}

}
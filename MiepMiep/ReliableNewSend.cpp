#include "ReliableNewSend.h"
#include "LinkStats.h"


namespace MiepMiep
{
	ReliableNewSend::ReliableNewSend(Link& link):
		ParentLink(link)
	{
	}

	MM_TS void ReliableNewSend::intervalDispatch( u64 time )
	{

	}
}
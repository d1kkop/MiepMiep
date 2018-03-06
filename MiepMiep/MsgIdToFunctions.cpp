#include "MsgIdToFunctions.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{

	MsgIdToFunctions::MsgIdToFunctions(Network& network):
		ParentNetwork(network)
	{
	}

	MsgIdToFunctions::~MsgIdToFunctions()
	= default;


}
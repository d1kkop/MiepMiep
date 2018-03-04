#pragma once

#include "MiepMiep.h"


namespace MyGame
{
	class Tree
	{
	public:
		Tree()
		{

		}

		MM_NETWORK_CREATABLE()
		{
			new Tree;
		}
	};
		
}

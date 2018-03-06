#pragma once

#include "MiepMiep.h"
using namespace MiepMiep;


namespace MyGame
{
	class Apple
	{
	public:
		Apple(INetwork& network)
		{
		}

		MM_NETWORK_CREATABLE()
		{
			new Apple( network );
		}


		NetInt32 m_Age;
		NetInt32 m_Color;
		NetUint32 m_Brand;
	};
		
}

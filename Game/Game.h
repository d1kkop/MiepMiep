#pragma once
#include "MiepMiep.h"
using namespace MiepMiep;


namespace MyGame
{
	class Game
	{
	public:
		Game();

		bool init();
		void run();


		sptr<INetwork> m_Network;
	};
}
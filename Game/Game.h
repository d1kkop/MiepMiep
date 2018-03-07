#pragma once
#include "MiepMiep.h"
using namespace MiepMiep;


namespace MyGame
{
	class Game : IConnectionListener
	{
	public:
		Game();

		bool init();
		void run();

		void onConnectResult( INetwork& network, const IEndpoint& etp, EConnectResult res ) override;

		sptr<INetwork> m_Network;
	};
}
#pragma once
#include "MiepMiep.h"
using namespace MiepMiep;


namespace MyGame
{
	class Game : IConnectionListener
	{
	public:
		Game();
		~Game() { stop(); }

		bool init();
		void run();
		void stop();

		void onRegisterResult( const ILink& link, bool result );
		void onJoinResult( const ILink& link, EJoinServerResult result );

		void onConnectResult( const ILink& link, EConnectResult res ) override;

		sptr<INetwork> m_Network;
	};
}
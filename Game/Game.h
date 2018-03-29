#pragma once
#include "MiepMiep.h"
using namespace MiepMiep;


namespace MyGame
{
	class Game : ISessionListener
	{
	public:
		Game();
		~Game() { stop(); }

		bool init();
		void run();
		void stop();

		void onRegisterResult( const ISession& session, bool result );
		void onJoinResult( const ISession& session, EJoinServerResult result );

		sptr<INetwork> m_Network;
	};
}
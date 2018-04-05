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
		void onJoinResult( const ISession& session, bool result );

		void onConnect( const ISession& session, const IAddress& remote ) override;
		void onDisconnect( const ISession& session, const IAddress& remote, EDisconnectReason reason ) override;
		void onOwnerChanged( const ISession& session, NetVar& variable, const IAddress* newOwner ) override;
		void onNewHost( const ISession& session, const IAddress* host ) override;
		void onLostHost( const ISession& session ) override;
		void onLostMasterLink( const ISession& session, const ILink& link ) override;

		sptr<INetwork> m_Network;
	};
}
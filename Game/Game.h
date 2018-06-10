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

		void onRegisterResult( ISession& session, bool result );
		void onJoinResult( ISession& session, bool result );

		void onConnect( ISession& session, const MetaData& md, const IAddress& remote ) override;
		void onDisconnect( ISession& session, const IAddress& remote, EDisconnectReason reason ) override;
		void onOwnerChanged( ISession& session, NetVar& variable, const IAddress* newOwner ) override;
		void onNewHost( ISession& session, const IAddress* host ) override;
		void onLostHost( ISession& session ) override;
		void onLostMasterLink( ISession& session, const ILink& link ) override;

		sptr<INetwork> m_Network;
	};
}
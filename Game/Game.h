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

		virtual void onConnect( const ISession& session, const IAddress& remote );
		virtual void onDisconnect( const ISession& session, const IAddress& remote, EDisconnectReason reason );
		virtual void onOwnerChanged( const ISession& session, NetVar& variable, const IAddress* newOwner );
		virtual void onNewHost( const ISession& session, const IAddress* host );
		virtual void onLostHost( const ISession& session );
		virtual void onLostMasterLink( const ISession& session, const ILink& link );

		sptr<INetwork> m_Network;
	};
}
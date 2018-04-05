#pragma once


#include "Memory.h"
#include "ParentNetwork.h"
#include "MiepMiep.h"
#include "NetworkEvents.h"
#include "Threading.h"


namespace MiepMiep
{
	class  Link;

	// ----- MasterSession client/server shared per link data  ------------------------------------------------------------------

	// TODO change for variable group instead!
	struct MasterSessionData
	{
		bool  m_IsP2p;
		bool  m_UsedMatchmaker;
		bool  m_IsPrivate;
		float m_Rating;
		u32	  m_MaxClients;
		bool  m_CanJoinAfterStart;
		std::string m_Name;
		std::string m_Type;
		std::string m_Password;
	};


	class SessionBase: public ParentNetwork, public ISession, public EventListener<ISessionListener>, public ITraceable
	{
	public:
		SessionBase(Network& network);
		MM_TS bool start();
		MM_TS const char* name() const override;

		MM_TS virtual void removeLink( Link& link ) = 0;
		MM_TS bool hasLink( const Link& link ) const;
		MM_TS void forLink( const Link* exclude, const std::function<void( Link& )>& cb ) const;
		MM_TSC const MasterSessionData& msd() const; // TODO retrieving members of this is not TS 

		MM_TO_PTR( SessionBase )

	protected:
		MM_TS virtual bool addLink( const sptr<Link>& link ) = 0;

	protected:
		u32 m_Id; // <-- Only used for debugging sessions
		mutable mutex m_DataMutex;
		bool m_Started;
		MasterSessionData m_MasterData;
		mutable mutex m_LinksMutex;
		vector<wptr<Link>> m_Links;

		friend class Link;
	};
}

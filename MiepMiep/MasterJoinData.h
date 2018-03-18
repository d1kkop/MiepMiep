#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentLink.h"
#include "Threading.h"
#include "MiepMiep.h"


namespace MiepMiep
{
	class MasterJoinData: public ParentLink, public IComponent, public IConnectionListener, public ITraceable
	{
	public:
		MasterJoinData(Link& link, const string& name, const string& pw, const string& type, float rating);
		~MasterJoinData() override;
		static EComponentType compType() { return EComponentType::MasterJoinData; }

		MM_TS void setServerProps( const MetaData& hostMd );
		MM_TS void setJoinFilter( const MetaData& joinMd, u32 minPlayers, u32 maxPlayers, float minRating, float maxRating );


		MM_TS void onConnectResult(const ILink& link, EConnectResult res) override;
		MM_TS void onNewConnection(const ILink& link, const IAddress& remote) override;
		MM_TS void onDisconnect(const ILink& link, EDisconnectReason reason, const IAddress& remote) override;

	private:
		mutable SpinLock m_DataMutex;
		string   m_Name, m_Type, m_Pw;
		float	 m_Rating;
		// Join filter
		float	 m_MinRating, m_MaxRating;
		u32		 m_MinPlayers, m_MaxPlayers;
		// Server custom md
		MetaData m_MetaData;
		bool	 m_IsRegister;
	};
}

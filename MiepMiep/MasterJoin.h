#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentLink.h"
#include "Threading.h"


namespace MiepMiep
{
	class MasterJoin: public ParentLink, public IComponent, public ITraceable
	{
	public:
		MasterJoin(Link& link, const string& name, const string& pw, const string& type, float rating);
		~MasterJoin() override;
		static EComponentType compType() { return EComponentType::MasterJoinData; }

		MM_TS void registerServer( const MetaData& customFilterMd );
		MM_TS void joinServer( u32 minPlayers, u32 maxPlayers, float minRating, float maxRating );

	private:
		mutable SpinLock m_DataMutex;
		string   m_Name, m_Type, m_Pw;
		float	 m_Rating;
	};
}

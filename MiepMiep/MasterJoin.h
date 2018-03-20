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
		MasterJoin(Link& link, const string& name, const string& pw, const string& type, float rating, const MetaData& md);
		~MasterJoin() override;
		static EComponentType compType() { return EComponentType::MasterJoinData; }

		// These are thread safe because they are set in the constructor. No locking is used here.
		MM_TS const string& name() const { return m_Name; }
		MM_TS const string& type() const { return m_Type; }
		MM_TS const string& password() const { return m_Pw; }
		MM_TS float initialRating() const { return m_Rating; }
		MM_TS const MetaData& metaData() const { return m_Md; }

		MM_TS void registerServer( bool isP2p, const MetaData& customFilterMd, 
								   const function<void (const ILink& link, bool)>& cb );
		MM_TS void joinServer( u32 minPlayers, u32 maxPlayers, float minRating, float maxRating, 
							   const function<void (const ILink& link, EJoinServerResult)>& cb );

		// Not thread safe, but set before request is transmitted. Only on reply the cb is requested.
		const function<void( const ILink& link, bool )>& getRegisterCb() const { return m_RegisterCb; }
		const function<void( const ILink& link, EJoinServerResult )>& getJoinCb() const { return m_JoinCb; }

	private:
		string   m_Name, m_Type, m_Pw;
		float	 m_Rating;
		MetaData m_Md;
		function<void (const ILink& link, bool)> m_RegisterCb;
		function<void (const ILink& link, EJoinServerResult)> m_JoinCb;
	};
}

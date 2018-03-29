#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentLink.h"
#include "Threading.h"


namespace MiepMiep
{
	struct MasterSessionData;
	struct SearchFilter;

	class MasterLinkData: public ParentLink, public IComponent, public ITraceable
	{
	public:
		MasterLinkData(Link& link);
		~MasterLinkData() override;
		static EComponentType compType() { return EComponentType::MasterLinkData; }

		MM_TS void registerServer( const function<void( const ISession&, bool )>& cb, const MasterSessionData& data, const MetaData& customMatchmakingMd );
		MM_TS void joinServer( const function<void( const ISession&, EJoinServerResult )>& cb, const SearchFilter& sf, const MetaData& customMatchmakingMd );

		// Not thread safe, but set before request is transmitted. Only on reply the cb is requested.
		const function<void( const ISession& session, bool )>& getRegisterCb() const { return m_RegisterCb; }
		const function<void( const ISession& session, EJoinServerResult )>& getJoinCb() const { return m_JoinCb; }

	private:
		mutex m_DataMutex;
		function<void (const ISession& link, bool)> m_RegisterCb;
		function<void (const ISession& link, EJoinServerResult)> m_JoinCb;
	};
}

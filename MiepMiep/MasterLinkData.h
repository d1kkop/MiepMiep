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

		void registerServer( const function<void( ISession&, bool )>& cb, const MasterSessionData& data, const MetaData& customMatchmakingMd );
		void joinServer( const function<void( ISession&, bool )>& cb, const SearchFilter& sf, const MetaData& customMatchmakingMd );

		// Not thread safe, but set before request is transmitted. Only on reply the cb is requested.
		const function<void( ISession& session, bool )>& getRegisterCb() const { return m_RegisterCb; }
		const function<void( ISession& session, bool )>& getJoinCb() const { return m_JoinCb; }

	private:
		function<void (ISession& link, bool)> m_RegisterCb;
		function<void (ISession& link, bool)> m_JoinCb;
	};
}

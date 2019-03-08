#pragma once

#include "SessionBase.h"


namespace MiepMiep
{
	class Link;
	struct MasterSessionData;


	class Session: public SessionBase
	{
	public:
		Session(Network& network, const MetaData& md=MetaData());
		MM_TSC void setMasterLink( const sptr<Link>& link );

		// ISession
		MM_TS sptr<const IAddress> host() const override;
		MM_TS sptr<ILink> matchMaker() const override;

		// SessionBase
		MM_TS void removeLink( Link& link ) override;

		MM_TS void updateHost( const sptr<const IAddress>& hostAddr );
		MM_TS void removeLink( const sptr<const Link>& link );
		MM_TS bool disconnect();

		// TODO change for variable group
		MM_TSC const MetaData& metaData() const { return m_MetaData; }

		MM_TO_PTR( Session )

	protected:
		sptr<const IAddress> m_Host;	// Note, for session this is an address whereas for the MasterSession this is a link!
		wptr<Link> m_MasterLink;		// Is also the packet handler for this session.
		MetaData   m_MetaData;
	};
}

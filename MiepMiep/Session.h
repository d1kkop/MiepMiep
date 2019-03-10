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
		void setMasterLink( const sptr<Link>& link );

		// ISession
		sptr<const IAddress> host() const override;
		sptr<ILink> matchMaker() const override;

		// SessionBase
		void removeLink( Link& link ) override;

		void updateHost( const sptr<const IAddress>& hostAddr );
		void removeLink( const sptr<const Link>& link );
		bool disconnect();

		// TODO change for variable group
		const MetaData& metaData() const { return m_MetaData; }

		MM_TO_PTR( Session )

	protected:
		sptr<const IAddress> m_Host;	// Note, for session this is an address whereas for the MasterSession this is a link!
		wptr<Link> m_MasterLink;		// Is also the packet handler for this session.
		MetaData   m_MetaData;
	};
}

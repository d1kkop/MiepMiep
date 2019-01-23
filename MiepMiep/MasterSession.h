#pragma once

#include "SessionBase.h"
#include <list>


namespace MiepMiep
{
	class Link;
	struct SearchFilter;
	class MasterSession;
	class MasterSessionList;

	template <typename T=MasterSessionData>
	bool readOrWrite( BinSerializer& bs, MasterSessionData& md, bool _write );

	// ----- MasterSession --------------------------------------------------------------------------------------------------------------

	using MsListCIt = list<sptr<MasterSession>>::const_iterator;

	class MasterSession : public SessionBase
	{
	public:
		MasterSession( const sptr<Link>& host, const MasterSessionData& data, const MasterSessionList& sessionList );

		// ISession
		MM_TS sptr<const IAddress> host() const override;

		// SessionBase
		MM_TS bool addLink( const sptr<Link>& link ) override;
		MM_TS void removeLink( Link& link ) override;

		MM_TS void updateHost( const sptr<Link>& link );
		MM_TS void removeSelf();
		MM_TS bool operator== ( const SearchFilter& sf ) const;

		// Becomes false if session was started in p2p and someone left or if was started and no late join is allowed.
		MM_TS bool canJoin() const;
		MM_TS void sendConnectRequests( Link& newLink );

		/*	Link to matchmaking server. */
		MM_TS sptr<ILink> matchMaker()  const override;

	private:
		wptr<Link> m_Host;				// Note: for session this is an address whereas for the MasterSession this is a link!
		bool m_SomeoneLeftTheSession;	// Becomes false if someone leaves in p2p session.

		// Hard link to session list, no Id's.
		wptr<MasterSessionList> m_SessionList;
		MsListCIt m_SessionListIt;

		friend class MasterSessionList;
	};
}

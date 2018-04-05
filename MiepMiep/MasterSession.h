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

		// SessionBase
		MM_TS sptr<const IAddress> host() const override;
		MM_TS void removeLink( Link& link ) override;

		MM_TS void updateHost( const sptr<Link>& link );
		MM_TS void removeSelf();
		MM_TS bool operator== ( const SearchFilter& sf ) const;
		bool canJoinNoLock() const;

		/*	Link to matchmaking server. */
		MM_TS const ILink& matchMaker()  const override;

	protected:
		MM_TS bool addLink( const sptr<Link>& link ) override;

	private:
		wptr<Link> m_Host;
		bool m_NewLinksCanJoin; // Becomes false if someone leaves in p2p session.

		// Hard link to session list, no Id's.
		sptr<MasterSessionList> m_SessionList;
		MsListCIt m_SessionListIt;

		friend class MasterSessionList;
	};
}

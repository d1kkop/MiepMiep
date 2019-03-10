#pragma once

#include "SessionBase.h"
#include "Link.h"
#include "Network.h"
#include "LinkManager.h"
#include "Platform.h"
#include <atomic>
#include <list>


namespace MiepMiep
{
	class ISocket;
	class IAddress;
	class MasterSession;
	struct SearchFilter;
	struct MasterSessionData;

	template <>
	bool readOrWrite( BinSerializer& bs, SearchFilter& sf, bool _write );


	// ----- SearchFilter data --------------------------------------------------------------------------------------------------------------

	struct SearchFilter
	{
		std::string m_Name;
		std::string m_Type;
		float m_MinRating, m_MaxRating;
		u32 m_MinPlayers, m_MaxPlayers;
		bool m_FindPrivate;
		bool m_FindP2p;
		bool m_FindClientServer;
		MetaData m_CustomMatchmakingMd;
	};


	// ----- MasterSessionList --------------------------------------------------------------------------------------------------------------

	using MsListCIt = list<sptr<MasterSession>>::const_iterator;

	class MasterSessionList : public ITraceable
	{
	public:
		~MasterSessionList() override;
		sptr<MasterSession> addSession( const sptr<Link>& host, const MasterSessionData& initialData );
		void removeSession( MsListCIt whereIt );
		u64 num() const;
		sptr<MasterSession> findFromFilter( const SearchFilter& sf );

	private:
		list<sptr<MasterSession>> m_MasterSessions;
	};


	class MasterSessionManager: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		MasterSessionManager(Network& network);
		~MasterSessionManager() override;
		static EComponentType compType() { return EComponentType::MasterServer; }

		sptr<MasterSession> registerSession( const sptr<Link>& link, const MasterSessionData& data );
		void removeSession( MasterSession& session );
		sptr<MasterSession> findServerFromFilter( const SearchFilter& sf );

	private:
		vector<sptr<MasterSessionList>> m_SessionLists;
	};
}
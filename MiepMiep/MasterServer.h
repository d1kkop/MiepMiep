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
		MM_TS sptr<MasterSession> addSession( const sptr<Link>& host, const MasterSessionData& initialData );
		MM_TS void removeSession( MsListCIt whereIt );
		MM_TS u64 num() const;
		MM_TS sptr<MasterSession> findFromFilter( const SearchFilter& sf );

	private:
		mutable mutex m_SessionsMutex;
		list<sptr<MasterSession>> m_MasterSessions;
	};


	class MasterServer: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		MasterServer(Network& network);
		~MasterServer() override;
		static EComponentType compType() { return EComponentType::MasterServer; }

		MM_TS sptr<MasterSession> registerSession( const sptr<Link>& link, const MasterSessionData& data );
		MM_TS void removeSession( MasterSession& session );
		MM_TS sptr<MasterSession> findServerFromFilter( const SearchFilter& sf );

	private:
		mutex m_ServerListMutex;
		vector<sptr<MasterSessionList>> m_SessionLists;
	};
}
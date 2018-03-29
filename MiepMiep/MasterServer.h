#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"
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

	template <typename T=MasterSessionData>
	bool readOrWrite( BinSerializer& bs, MasterSessionData& md, bool _write );



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


	// ----- MasterSession client/server shared per link data  ------------------------------------------------------------------

	// TODO change for variable group instead!
	struct MasterSessionData : public ITraceable
	{
		bool  m_IsP2p;
		bool  m_UsedMatchmaker;
		bool  m_IsPrivate;
		float m_Rating;
		u32	  m_MaxClients;
		bool  m_CanJoinAfterStart;
		std::string m_Name;
		std::string m_Type;
		std::string m_Password;
	};


	// ----- MasterSession --------------------------------------------------------------------------------------------------------------

	using MsListCIt = list<sptr<MasterSession>>::const_iterator;

	class MasterSession : public ITraceable
	{
	public:
		MasterSession( const sptr<Link>& host, const MasterSessionData& data, const MasterSessionList& sessionList, const MetaData& customMatchmakingMd );

		MM_TS bool start();
		MM_TS bool tryJoin( Link& link, const MetaData& customMatchmakingMd );
		MM_TS bool onClientLeaves( Link& link );
		MM_TS void removeSelf();
		MM_TS bool operator== ( const SearchFilter& sf ) const;
		bool canJoinNoLock() const;

		//	bool readOrWrite( BinSerializer& bs, bool write );

	private:
		mutable mutex m_DataMutex;
		bool m_Started;
		MasterSessionData m_Data;
		sptr<Link> m_Host;
		vector<sptr<Link>> m_Links;
		bool m_NewLinksCanJoin; // Becomes false if someone leaves in p2p session.

		// Hard link to session list, no Id's.
		sptr<MasterSessionList> m_SessionList;
		MsListCIt m_SessionListIt;

		friend class MasterSessionList;
	};


	// ----- MasterSessionList --------------------------------------------------------------------------------------------------------------

	class MasterSessionList : public ITraceable
	{
	public:
		MM_TS void addSession( const sptr<Link>& host, const MasterSessionData& initialData, const MetaData& customMatchmakingMd );
		MM_TS void removeSession( MsListCIt whereIt );
		MM_TS u64 num() const;
		MM_TS MasterSession* findFromFilter( const SearchFilter& sf );

	private:
		mutable mutex m_SessionsMutex;
		list<sptr<MasterSession>> m_MasterSessions;
	};


	class MasterServer: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		MasterServer(Network& network);
		static EComponentType compType() { return EComponentType::MasterServer; }

		MM_TS bool registerSession( const sptr<Link>& link, const MasterSessionData& data, const MetaData& customMatchmakingMd );
		MM_TS void removeSession( MasterSession& session );
		MM_TS MasterSession* findServerFromFilter( const SearchFilter& sf );

	private:
		mutex m_ServerListMutex;
		vector<sptr<MasterSessionList>> m_SessionLists;
	};
}
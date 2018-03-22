#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"
#include "Socket.h"
#include "LinkManager.h"
#include "Network.h"


namespace MiepMiep
{
	class ISocket;
	class IAddress;


	template <>
	inline bool readOrWrite( BinSerializer& bs, SearchFilter& sf, bool _write )
	{
		__CHECKEDB( bs.readOrWrite( sf.m_Type, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_Name, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MinRating, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MinPlayers, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MaxRating, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_MaxPlayers, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_FindPrivate, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_FindP2p, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_FindClientServer, _write ) );
		__CHECKEDB( bs.readOrWrite( sf.m_CustomMatchmakingMd, _write ) );
		return false;
	}

	template <>
	inline bool readOrWrite( BinSerializer& bs, MasterSessionData& md, bool _write )
	{
		return false;
	}


	// ----- MasterSession --------------------------------------------------------------------------------------------------------------

	class MasterSession
	{
	public:
		MasterSession( const MasterSessionData& data );

		MM_TS bool operator== (const SearchFilter& sf) const;

	//	bool readOrWrite( BinSerializer& bs, bool write );

	private:
		MasterSessionData m_Data;
		mutable mutex m_DataMutex;

		sptr<Link> m_Host;
		vector<sptr<Link>> m_Links;
	};


	// ----- ServerList --------------------------------------------------------------------------------------------------------------

	class ServerList : public ITraceable
	{
	public:
		MM_TS void addServer( const sptr<MasterSessionData>& data );
		MM_TS void removeServer( const sptr<MasterSessionData>& data );
		MM_TS u64 num() const;
		MM_TS sptr<MasterSession> findFromFilter( const SearchFilter& sf );

	private:
		mutable mutex m_ServersMutex;
		map<u64, sptr<MasterSession>> m_MasterSessions;
	};


	class MasterServer: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		MasterServer(Network& network);
		static EComponentType compType() { return EComponentType::MasterServer; }

		MM_TS bool registerServer( const sptr<Link>& link, const MasterSessionData& data );
		MM_TS void removeServer( const sptr<Link>& link ); // Must be link that is part of a session.
		MM_TS SocketAddrPair findServerFromFilter( const SearchFilter& sf );

	private:
		atomic<u64> m_MasterSessionIdNumerator;
		mutex m_ServerListMutex;
		vector<sptr<ServerList>> m_ServerLists;
	};
}
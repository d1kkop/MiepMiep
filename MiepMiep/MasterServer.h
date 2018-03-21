#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"
#include "Socket.h"
#include "LinkManager.h"


namespace MiepMiep
{
	class ISocket;
	class IAddress;

	// ----- SearchFilter --------------------------------------------------------------------------------------------------------------

	struct SearchFilter
	{
		SearchFilter() { memset( this, 0, sizeof( *this ) ); }

		string m_Name;
		string m_Type;
		float m_MinRating, m_MaxRating;
		u32 m_MinPlayers, m_MaxPlayers;
		bool m_FindPrivate;
		bool m_FindP2p;
		bool m_FindClientServer;
	};

	// ----- MasterSession --------------------------------------------------------------------------------------------------------------

	class MasterSession
	{
	public:
		MasterSession() = default; // Calloc makes it all zero.
		MasterSession(bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customFilterMd);

		MM_TS bool operator== (const SearchFilter& sf) const;

	private:
		bool m_IsP2p;
		bool m_IsPrivate;
		string m_Name;
		string m_Type;
		float  m_Rating;
		u32 m_MaxClients;
		MetaData m_CustomMd;
		mutable mutex m_DataMutex;

		sptr<Link> m_Host;
		vector<sptr<Link>> m_Links;
	};


	// ----- ServerList --------------------------------------------------------------------------------------------------------------

	class ServerList
	{
	public:
		MM_TS void add( const SocketAddrPair& sap, bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customFitlerMd );
		MM_TS bool exists( const SocketAddrPair& sap ) const;
		MM_TS u64 num() const;
		MM_TS SocketAddrPair findFromFilter( const SearchFilter& sf );

	private:
		mutable mutex m_ServersMutex;
		map<SocketAddrPair, MasterSession> m_Servers;
	};


	class MasterServer: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		MasterServer(Network& network);
		static EComponentType compType() { return EComponentType::MasterServer; }

		MM_TS bool registerServer( const ISocket& reception, const IAddress& addr, bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customFilterMd );
		MM_TS SocketAddrPair findServerFromFilter( const SearchFilter& sf );

	private:
		mutex m_ServerListMutex;
		vector<sptr<ServerList>> m_ServerLists;
	};
}
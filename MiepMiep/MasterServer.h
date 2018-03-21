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


	class MasterSession
	{
	public:
		MasterSession() = default; // Calloc makes it all zero.
		MasterSession(bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customFilterMd);

	private:
		bool   m_IsP2p, m_IsPrivate;
		string m_Name, m_Pw, m_Type;
		float  m_Rating;
		MetaData m_CustomFilterMd;
		sptr<Link> m_Host;
		vector<sptr<Link>> m_Links;

		friend class ServerList;
	};


	class ServerList
	{
	public:
		MM_TS void add( const SocketAddrPair& sap, bool isP2p, const string& name, const string& type, float initialRating, const MetaData& customFitlerMd );
		MM_TS bool exists( const SocketAddrPair& sap ) const;
		MM_TS u64 count() const;
		MM_TS SocketAddrPair findFromFilter( const string& name, const string& pw, const string& type, float minRating, float maxRating, u32 minPlayers, u32 maxPlayers);

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
		MM_TS SocketAddrPair findServerFromFilter( const string& name, const string& pw, const string& type, float minRating, float maxRating, u32 minPlayers, u32 maxPlayers );

	private:
		mutex m_ServersMutex;
		vector<sptr<ServerList>> m_ServerLists;
	};
}
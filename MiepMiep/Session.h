#pragma once


#include "Memory.h"
#include "ParentNetwork.h"
#include "MiepMiep.h"
#include "Threading.h"
#include "NetworkEvents.h"
#include <mutex>
#include <vector>


namespace MiepMiep
{
	class Link;
	struct MasterSessionData;


	class Session: public ParentNetwork, public ISession, public EventListener<ISessionListener>, public ITraceable
	{
	public:
		Session(Network& network, const sptr<Link>& masterLink, const MetaData& md=MetaData());

		// ISession
		MM_TS const ILink& matchMaker() const override;
		MM_TS const IAddress* host() const override;

		MM_TS void addLink( const sptr<Link>& link );
		MM_TS void removeLink( const sptr<const Link>& link );
		MM_TS void removeLink( const Link& link );
		MM_TS bool hasLink( const Link& link ) const;
		MM_TS void forLink( const Link* exclude, const std::function<void (Link&)>& cb ) const;
		MM_TS bool disconnect();

		// TODO change for varaible group
		MM_TSC const MetaData& metaData() const { return m_MetaData; }
		MM_TSC const MasterSessionData& msd() const;

		MM_TO_PTR( Session )

	private:
		mutable SpinLock m_DataMutex;
		sptr<MasterSessionData> m_MasterData;
		sptr<Link> m_MasterLink;		// Is also the packet handler for this session.
		sptr<const IAddress> m_Host;	// Host in cl-serv of super peer inp2p.
		MetaData m_MetaData;
		mutable mutex m_LinksMutex;
		vector<sptr<Link>> m_Links;
	};
}

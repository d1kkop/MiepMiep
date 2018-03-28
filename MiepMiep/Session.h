#pragma once


#include "Memory.h"
#include "ParentNetwork.h"
#include "MiepMiep.h"
#include <mutex>
#include <vector>


namespace MiepMiep
{
	class Link;


	class Session: public ParentNetwork, public ISession, public ITraceable
	{
	public:
		Session(Network& network, const sptr<Link>& masterLink, const string& pw, const MetaData& md=MetaData());

		MM_TS void addLink( const sptr<Link>& link );
		MM_TS void removeLink( const sptr<Link>& link );
		MM_TS void forLink( const Link* exclude, const std::function<void (Link&)>& cb ) const;

		MM_TO_PTR( Session )

	private:
		mutex m_DataMutex;
		sptr<Link> m_MasterLink; // Is also the packet handler for this session.
		string m_Pw;
		MetaData m_MetaData;
		mutable mutex m_LinksMutex;
		vector<sptr<Link>> m_Links;
	};
}

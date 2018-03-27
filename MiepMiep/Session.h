#pragma once


#include "Memory.h"
#include "MiepMiep.h"
#include <mutex>
#include <vector>


namespace MiepMiep
{
	class Link;


	class Session: public ISession, public ITraceable
	{
	public:
		Session(const sptr<Link>& masterLink, const string& pw, const MetaData& md=MetaData());

		MM_TS void addLink( const sptr<Link>& link );
		MM_TS void removeLink( const sptr<Link>& link );

	private:
		mutex m_DataMutex;
		sptr<Link> m_MasterLink; // Is also the packet handler for this session.
		vector<sptr<Link>> m_Links;
		string m_Pw;
		MetaData m_MetaData;
	};
}

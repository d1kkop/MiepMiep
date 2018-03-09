#include "MasterJoinData.h"


namespace MiepMiep
{

	MasterJoinData::MasterJoinData(Link& link):
		ParentLink(link)
	{
	}

	MM_TS void MasterJoinData::setName(const string& name)
	{
		scoped_lock lk(m_DataMutex);
		m_Name = name;
	}

	MM_TS void MasterJoinData::setMetaData(const MetaData& md)
	{
		scoped_lock lk(m_DataMutex);
		m_MetaData = md;
	}

	MM_TS string MasterJoinData::name() const
	{
		scoped_lock lk(m_DataMutex);
		return m_Name;
	}

	MM_TS MetaData MasterJoinData::metaData() const
	{
		scoped_lock lk(m_DataMutex);
		return m_MetaData;
	}

}
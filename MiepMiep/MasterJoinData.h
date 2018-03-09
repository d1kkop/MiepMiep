#pragma once

#include "Component.h"
#include "Link.h"


namespace MiepMiep
{
	class MasterJoinData: public ParentLink, public IComponent, public ITraceable
	{
	public:
		MasterJoinData(Link& link);
		static EComponentType compType() { return EComponentType::MasterJoinData; }

		MM_TS void setName( const string& name );
		MM_TS void setMetaData( const MetaData& md );

		MM_TS string name() const;
		MM_TS MetaData metaData() const;

	private:
		mutable mutex m_DataMutex;
		string m_Name;
		MetaData m_MetaData;
	};
}

#include "SessionBase.h"
#include "Link.h"
#include "SessionBase.h"
#include "MasterServer.h"
#include "Util.h"
#include <algorithm>


namespace MiepMiep
{
	// --------- SessionBase ----------------------------------------------------------------------------------------------

	SessionBase::SessionBase( Network& network ):
		ParentNetwork( network ),
		m_Id( Util::rand() ),
		m_Started(false)
	{
	}

	MM_TS bool SessionBase::start()
	{
		scoped_lock lk( m_DataMutex );
		if ( !m_Started )
		{
			m_Started = true;
			return true;
		}
		return false;
	}

	MM_TS const char * SessionBase::name() const
	{
		// TODO
		return "";
	}

	MM_TS bool SessionBase::hasLink( const Link& link ) const
	{
		scoped_lock lk( m_LinksMutex );
		for ( auto& l : m_Links )
		{
			if ( *l == link ) return true;
		}
		return false;
	}

	MM_TS void SessionBase::forLink( const Link* exclude, const std::function<void( Link& )>& cb ) const
	{
		scoped_lock lk( m_LinksMutex );
		for ( auto& l : m_Links )
		{
			if ( l.get() == exclude ) continue;
			cb( *l );
		}
	}

	MM_TSC const MasterSessionData& SessionBase::msd() const
	{
		return m_MasterData;
	}


	MM_TO_PTR_IMP( SessionBase )
}
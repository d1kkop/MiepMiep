#include "SessionBase.h"
#include "Link.h"
#include "SessionBase.h"
#include "MasterServer.h"
#include "Util.h"
#include "ReliableSend.h"
#include "Endpoint.h"
#include <algorithm>


namespace MiepMiep
{
	// --------- SessionBase ----------------------------------------------------------------------------------------------

	SessionBase::SessionBase( Network& network ):
		ParentNetwork( network ),
		m_Id( network.nextSessionId() ),
		m_Started(false)
	{
	}

	MM_TSC INetwork& SessionBase::network() const
	{
		return m_Network;
	}

    MM_TSC u32 SessionBase::id() const
    {
        return m_Id;
    }

    MM_TSC const MasterSessionData& SessionBase::msd() const
	{
		// MasterSessionReference set at construction, no lock requied.
		return m_MasterData;
	}

	MM_TS bool SessionBase::start()
	{
		rscoped_lock lk( m_DataMutex );
		if ( !m_Started )
		{
			m_Started = true;
			return true;
		}
		return false;
	}

	MM_TS const char * SessionBase::name() const
	{
		rscoped_lock lk(m_DataMutex);
		return m_MasterData.m_Name.c_str();
	}

	MM_TS void SessionBase::bufferMsg( const vector<sptr<const NormalSendPacket>>& data, byte channel )
	{
		assert( channel <= MM_CHANNEL_MASK );
		auto& buffQueue = m_BufferedPackets[channel];
		rscoped_lock lk( m_DataMutex );
		for ( auto& dataPack : data )
		{
			buffQueue.emplace_back( dataPack );
		}
	}

	MM_TS bool SessionBase::addLink( const sptr<Link>& link )
	{
		assert( !hasLink( *link ) );
		LOG( "Added new link %s in sessionId %d.", link->info(), m_Id );
		// Make buffering packets and adding the new link a single atomic process to ensure
		// no discrepanties arise between sending buffered packets to new and existing links and adding new buffered packets.
		rscoped_lock lk2( m_DataMutex );
		for ( u32 ch=0; ch<MM_NUM_CHANNELS; ch++)
		{
			sptr<ReliableSend> rd = link->getOrAdd<ReliableSend>( ch );
			for ( auto& buffPack : m_BufferedPackets[ch] )
			{
				rd->enqueue( buffPack, No_Trace );
			}
		}
		m_Links.emplace_back( link );
        link->setSession( *this );
		return true;
	}

	MM_TS bool SessionBase::hasLink( const Link& link ) const
	{
		rscoped_lock lk( m_DataMutex );
		for ( auto& l : m_Links )
		{
			auto sl = l.lock();
			if ( !sl ) continue;
			if ( *sl == link ) return true;
		}
		return false;
	}

	MM_TS void SessionBase::forLink( const Link* exclude, const std::function<void( Link& )>& cb )
	{
		rscoped_lock lk( m_DataMutex );
		// Auto cleanup empty links
		for ( auto it = m_Links.begin(); it != m_Links.end(); )
		{
			auto sl = (*it).lock();
			if ( !sl )
			{
				// remove
				it = m_Links.erase( it );
			}
			else
			{
				it++;
				if ( sl.get() != exclude )
				{
					cb( *sl );
				}
			}
		}
	}


	MM_TO_PTR_IMP( SessionBase )
}
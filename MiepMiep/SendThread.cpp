#include "SendThread.h"
#include "Link.h"
#include "Network.h"
#include "LinkManager.h"
#include "ReliableSend.h"
#include "ReliableNewSend.h"
#include "ReliableAckSend.h"
#include "Util.h"
#include "Platform.h"
using namespace chrono;


namespace MiepMiep
{

	// -------- SendThread -------------------------------------------------------------------------------------

	SendThread::SendThread(Network& network):
		ParentNetwork(network),
		m_Closing(false)
	{
		start();
	}

	SendThread::~SendThread()
	{
		stop();
	}

	void SendThread::start()
	{
		assert (!m_Closing);
		m_SendThread = thread([this]() {
			resendThread();
		});
	}

	void SendThread::stop()
	{
		m_Closing = true;
		if ( m_SendThread.joinable() )
			m_SendThread.join();
	}

	void SendThread::resendThread()
	{
		set_terminate([]()
		{ 
			LOGC( "Sendthread encountered unhandled exception. Terminating thread." );
			abort();
		});

		while ( true )
		{
			Platform::sleep( MM_ST_RESEND_CHECK_INTERVAL ); // In ms
			if ( isClosing() )
				return;

			u64 time = Util::abs_time();
			auto lm = m_Network.get<LinkManager>();
			if ( !lm )
				continue;

			// Per N links create an async job and resend if necessary
			lm->forEachLink( [=](Link& link)
			{
				intervalDispatchOnAllChannels<ReliableSend>( link, time );
				intervalDispatchOnAllChannels<ReliableNewSend>( link, time );
				intervalDispatchOnAllChannels<ReliableAckSend>( link, time );
			}, MM_ST_LINKS_CLUSTER_SIZE );
		}
	}

}
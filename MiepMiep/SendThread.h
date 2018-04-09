#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"
#include "Link.h"

//// Streams
//#include "ReliableSend.h"
//#include "ReliableAckSend.h"
//#include "ReliableNewSend.h"


namespace MiepMiep
{
	class SendThread: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		SendThread(Network& network);
		~SendThread() override;
		static EComponentType compType() { return EComponentType::SendThreadManager; }
		bool isClosing() const volatile { return m_Closing; }
		void start();
		void stop();
		void resendThread();

	private:
		template <typename T> 
		inline void intervalDispatchOnAllChannels( Link& link, u64 time );


	private:
		bool m_Closing;
		thread m_SendThread;
	};


	template <typename T>
	void SendThread::intervalDispatchOnAllChannels( Link& link, u64 time )
	{
		u32 num = link.count<T>();
		for ( u32 i=0; i<num; i++ )
		{
			if ( auto s = link.get<T>( i ) )
			{
				s->intervalDispatch( time );
			}
		}
	}

}
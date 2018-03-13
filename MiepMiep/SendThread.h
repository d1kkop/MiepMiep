#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentNetwork.h"


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
		bool m_Closing;
		thread m_SendThread;
	};
}
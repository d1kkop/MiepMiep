#include "NetworkActions.h"
#include "Network.h"
#include "Link.h"


namespace MiepMiep
{
	NetworkActions::NetworkActions(Network& network):
		ParentNetwork(network),
		m_Closing(false)
	{
		m_ActionThread = thread([this]() 
		{
			actionThread();
		});
	}

	NetworkActions::~NetworkActions()
	{
		m_Closing = true;
		if (m_ActionThread.joinable())
		{
			m_ActionsMutex.lock();
			m_Actions.clear();
			m_ActionsCv.notify_one();
			m_ActionsMutex.unlock();
			m_ActionThread.join();
		}
	}

	void NetworkActions::addAction(sptr<IAction>& action)
	{
		scoped_lock lk(m_ActionsMutex);
		m_Actions.push_back(action);
		m_ActionsCv.notify_one();
	}

	void NetworkActions::actionThread()
	{
		while (!m_Closing)
		{
			unique_lock<mutex> lk(m_ActionsMutex);
			if (m_Actions.empty())
			{
				m_ActionsCv.wait(lk);
			}
			for (auto& a : m_Actions)
			{
				processAction(*a);
			}
			m_Actions.clear();
			if (lk.owns_lock())
			{
				lk.unlock();
			}
		}
	}

	void NetworkActions::processAction(const IAction& a)
	{
		switch (a.m_Type)
		{
		case ActionType::AddLink:
			break;
		case ActionType::RemoveLink:
			break;
		default:
			break;

		}
	}

}
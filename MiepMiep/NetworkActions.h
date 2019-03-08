#pragma once

#include "Component.h"
#include "Memory.h"
#include "ParentNetwork.h"
#include <algorithm>
#include <list>


namespace MiepMiep
{
	class Link;

	// -------- Polymorphic Events --------------------------------------------------------------------------------------------

	enum class ActionType
	{
		AddLink,
		RemoveLink
	};

	struct IAction : public ITraceable
	{
		IAction(const sptr<Link>& link, ActionType type) :
			m_Link(link),
			m_Type(type)
		{
		}

		virtual void process() = 0;

		sptr<Link> m_Link;
		ActionType m_Type;
	};
	

	// -------- NetworkEvents --------------------------------------------------------------------------------------------

	class NetworkActions: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		NetworkActions(Network& network);
		virtual ~NetworkActions() override;
		static EComponentType compType() { return EComponentType::NetworkActions; }

		MM_TS void addAction( sptr<IAction>& action );
		
	private:
		MM_TS void actionThread();
		void processAction(const IAction& a);

		mutex m_ActionsMutex;
		condition_variable m_ActionsCv;
		list<sptr<IAction>> m_Actions;
		volatile bool m_Closing;
		thread m_ActionThread;
	};



}
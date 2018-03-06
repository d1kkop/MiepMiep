#pragma once


#include "Network.h"
#include <mutex>
#include <map>
#include <functional>
using namespace std;


namespace MiepMiep
{
	using RpcFunc		  = function<void (class BinSerializer&, class INetwork&, const class IEndpoint*)>;
	using GroupCreateFunc = function<void (class INetwork&, const class IEndpoint&)>;


	class MsgIdToFunctions: public ParentNetwork, public IComponent, public ITraceable
	{
	public:
		MsgIdToFunctions(Network& network);
		~MsgIdToFunctions() override;
		static EComponentType compType() { return EComponentType::GroupCreateFunctions; }


	private:
		mutex m_RpcMutex;
		mutex m_GroupFuncMapMutex;
		map<u16, RpcFunc> m_RpcFuncMap;
		map<u16, GroupCreateFunc> m_GroupCreateMap;
	};

}
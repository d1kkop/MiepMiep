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


	class MsgIdToFunctions: public Parent<Network>, public IComponent, public ITraceable
	{
	public:
		MsgIdToFunctions(Network& network);
		~MsgIdToFunctions() override;
		static EComponentType compType() { return EComponentType::GroupCreateFunctions; }

		MM_TS void registerRpc(u16 id, const RpcFunc& cb);
		MM_TS void deregisterRpc(u16 id);
		MM_TS void callRpc(u16 rpcType, const IEndpoint* specific, bool exclude, bool buffer);

		MM_TS void registerGroup(u16 id, const GroupCreateFunc& cb);
		MM_TS void deregisterGroup(u16 id);


	private:
		mutex m_RpcMutex;
		mutex m_GroupFuncMapMutex;
		map<u16, RpcFunc> m_RpcFuncMap;
		map<u16, GroupCreateFunc> m_GroupCreateMap;
	};

}
#include "MsgIdToFunctions.h"
#include "PerThreadDataProvider.h"


namespace MiepMiep
{

	MsgIdToFunctions::MsgIdToFunctions(Network& network):
		Parent(network)
	{
	}

	MsgIdToFunctions::~MsgIdToFunctions()
	= default;

	MM_TS void MsgIdToFunctions::registerRpc(u16 id, const RpcFunc& func)
	{
		scoped_lock lk(m_RpcMutex);
		__CHECKED( m_RpcFuncMap.count(id) == 0 );
		m_RpcFuncMap[id] = func;
	}

	MM_TS void MsgIdToFunctions::deregisterRpc(u16 id)
	{
		scoped_lock lk(m_RpcMutex);
		__CHECKED( m_RpcFuncMap.count(id) == 1 );
		m_RpcFuncMap.erase( id );
	}

	MM_TS void MsgIdToFunctions::callRpc(u16 id, const IEndpoint* specific, bool exclude, bool buffer)
	{
		auto& bs = PerThreadDataProvider::beginSend();
		scoped_lock lk(m_RpcMutex);
		__CHECKED( m_RpcFuncMap.count(id) == 1 );
		auto rpcIt  = m_RpcFuncMap.find( id );
		if ( rpcIt != m_RpcFuncMap.end() )
		{
			(rpcIt->second)( bs, m_Parent, nullptr  ); // is writing
		}
	}

	MM_TS void MsgIdToFunctions::registerGroup(u16 id, const GroupCreateFunc& func)
	{
		scoped_lock lk(m_GroupFuncMapMutex);
		__CHECKED( m_GroupCreateMap.count(id) == 0 );
		m_GroupCreateMap[id] = func;
	}

	MM_TS void MsgIdToFunctions::deregisterGroup(u16 id)
	{
		scoped_lock lk(m_GroupFuncMapMutex);
		__CHECKED( m_GroupCreateMap.count(id) == 1 );
		m_GroupCreateMap.erase( id );
	}

}
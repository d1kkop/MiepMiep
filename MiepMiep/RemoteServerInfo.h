#pragma once

#include "Component.h"
#include "Memory.h"
#include "Endpoint.h"


namespace MiepMiep
{
	class RemoteServerInfo: public IComponent, public ITraceable
	{
	public:
		static EComponentType compType() { return EComponentType::RemoteServerInfo; }

	private:
		string m_Name;
		sptr<IEndpoint> m_Endpoint;
	};
}

#pragma once

#include "Memory.h"
#include "Component.h"
#include "ParentLink.h"


namespace MiepMiep
{
	class Link;

	struct Block
	{

	};

	struct BlockList
	{
		vector<Block> m_Blocks;
	};


	class ReliableNewSend: public ParentLink, public IComponent, public ITraceable
	{
	public:
		ReliableNewSend(Link& link);
		static EComponentType compType() { return EComponentType::ReliableNewSend; }

		MM_TS void intervalDispatch( u64 time );
	};
}

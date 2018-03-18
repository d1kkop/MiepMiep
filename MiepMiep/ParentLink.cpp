#include "ParentLink.h"
#include "Network.h"
#include "Link.h"


namespace MiepMiep
{
	ParentLink::ParentLink(Link& link):
		m_Link(link) 
	{
	}

	Network& ParentLink::network()
	{
		return m_Link.m_Network;
	}

}
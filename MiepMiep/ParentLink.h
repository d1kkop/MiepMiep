#pragma once


namespace MiepMiep
{
	class Link;
	class Network;

	class ParentLink
	{
	public:
		ParentLink(Link& link);
		Link& m_Link;
		Network& network();
	};
}
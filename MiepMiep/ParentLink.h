#pragma once


namespace MiepMiep
{
	class Link;

	class ParentLink
	{
	public:
		ParentLink(Link& link);
		Link& m_Link;
	};
}
#pragma once


namespace MiepMiep
{
	class Network;

	class ParentNetwork
	{
	public:
		ParentNetwork(Network& network);

		Network& m_Network;
	};
}
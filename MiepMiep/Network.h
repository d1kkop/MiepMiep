#pragma once

#include "MiepMiep.h"
#include "Memory.h"
#include "Component.h"
using namespace std;


namespace MiepMiep
{
	class Link;

	enum class ENetworkError
	{
		Fine,
		SerializationError,
		LogicalError
	};


	// ------------ Network -----------------------------------------------


	class Network: public ComponentCollection, public INetwork, public ITraceable
	{
	public:
		Network();
		~Network() override;

		void processEvents() override;

		// INetwork
		MM_TS ERegisterServerCallResult registerServer( const IEndpoint& masterEtp, const string& serverName, const string& pw="", const MetaData& md=MetaData() ) override;
		MM_TS EJoinServerCallResult joinServer( const IEndpoint& masterEtp, const string& serverName, const string& pw="", const MetaData& md=MetaData() ) override;

		MM_TS void createGroupInternal( const string& typeName, const BinSerializer& initData, byte channel, IDeliveryTrace* trace );
		MM_TS void destroyGroup( u32 groupId ) override;

		MM_TS void createRemoteGroup( const string& typeName, u32 netId, const BinSerializer& initData, const IEndpoint& etp );

		MM_TS ESendCallResult sendReliable(BinSerializer& bs, const IEndpoint* specific, 
										   bool exclude, bool buffer, bool relay,
										   byte channel, IDeliveryTrace* trace) override;

		MM_TS void addConnectionListener( IConnectionListener* listener ) override;
		MM_TS void removeConnectionListener( const IConnectionListener* listener ) override;


		MM_TS static void clearAllStatics();
		MM_TS MM_DECLSPEC static void printMemoryLeaks();

		// Gets, checks or adds directly in a link in the network -> linkManagerComponent.
		MM_TS sptr<Link> getLink(const IEndpoint& etp) const;
		template <typename T>
		MM_TS bool hasOnLink(const IEndpoint& etp, byte idx=0) const;
		template <typename T>
		MM_TS sptr<T> getOnLink(const IEndpoint& etp, byte idx=0) const;
		template <typename T>
		MM_TS sptr<T> getOrAddOnLink(const IEndpoint& etp, byte idx=0);

		// Gets or adds component to ComponentCollection.
		template <typename T, typename ...Args>
		MM_TS sptr<T> getOrAdd(byte idx=0, Args... args);

	};


	template <typename T>
	MM_TS bool MiepMiep::Network::hasOnLink(const IEndpoint& etp, byte idx) const
	{
		const sptr<Link>& link = getLink(etp);
		if ( !link ) return false;
		return link->has<T>(idx);
	}

	template <typename T>
	MM_TS sptr<T> MiepMiep::Network::getOnLink(const IEndpoint& etp, byte idx) const
	{
		Link* link = getLink(etp);
		if ( !link ) return nullptr;
		return link->get<T>(idx);
	}

	template <typename T>
	MM_TS sptr<T> MiepMiep::Network::getOrAddOnLink(const IEndpoint& etp, byte idx)
	{
		Link* link = getLink(etp);
		if ( !link ) return nullptr;
		return link->getOrAdd<>T();
	}


	template <typename T, typename ...Args>
	MM_TS sptr<T> MiepMiep::Network::getOrAdd(byte idx, Args... args)
	{
		return getOrAddInternal<T, Network&>(idx, *this, args...);
	}


	template <typename T>
	Network& toNetwork(T& t) 
	{ 
		return static_cast<Network&>(t);
	}


	// ------------ ParentNetwork -----------------------------------------------

	class ParentNetwork
	{
	public:
		ParentNetwork(Network& network):
			m_Network(network) { }

		Network& m_Network;
	};
}
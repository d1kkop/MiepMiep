#pragma once

#include "Variables.h"
#include "Rpc.h"
#include <string>


namespace MiepMiep
{
	// ---------- Enums -------------------------------
	
	enum class EListenCallResult
	{
		Fine,
		SocketError
	};


	enum class ERegisterServerCallResult
	{
		Fine,
		AlreadyRegistered
	};


	enum class EJoinServerCallResult
	{
		Fine,
		AlreadyJoined
	};

	enum class ESendCallResult
	{
		/*	The packet is, or will be sent soon. It does not mean the packet is delivered. */
		Fine,
		/*	Reasons for not sent are:
			1. The (internal) list of (connected) endpoints is empty.
			2. The number of connected endpoints is 1 and is the same as the endpoint to exclude.
			3. The destination was a closed connection and therefore the send was blocked.  */
		NotSent,
		/*	See log for more info. */
		SerializationError,
		InternalError
	};


	// ---------- User Classes -------------------------------

	class MM_DECLSPEC IDeliveryTrace
	{
	};


	class MM_DECLSPEC IEndpoint
	{
	public:
		static sptr<IEndpoint> resolve( const std::string& name, u16 port, i32* errOut=nullptr );
		static sptr<IEndpoint> fromIpAndPort( const std::string& ipAndPort, i32* errOut=nullptr );

		virtual std::string toIpAndPort() const = 0;
		virtual sptr<IEndpoint> getCopy() const = 0;

		bool operator==( const IEndpoint& other ) const;
		bool operator==( const sptr<IEndpoint>& other )	const		{ return *this == *other; }
		bool operator!=( const IEndpoint& other ) const				{ return !(*this == other); }
		bool operator!=( const sptr<IEndpoint>& other )				{ return !(*this == *other); }

		struct stl_less
		{
			bool operator()( const sptr<IEndpoint>& left, const sptr<IEndpoint>& right ) const;

		};
	};


	class MM_DECLSPEC INetwork
	{
	public:
		virtual ~INetwork() = default;
		MM_TS static  sptr<INetwork> create();

		MM_TS virtual ERegisterServerCallResult registerServer( const IEndpoint& masterEtp, const std::string& serverName, const std::string& pw="", const MetaData& md=MetaData() ) = 0;
		MM_TS virtual EJoinServerCallResult joinServer( const IEndpoint& masterEtp, const std::string& serverName, const std::string& pw="", const MetaData& md=MetaData() ) = 0;

		template <typename T, typename ...Args> 
		MM_TS ESendCallResult callRpc( Args... args, bool localCall=false, const IEndpoint* specific=nullptr, bool exclude=false, bool buffer=false, 
									   bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr );

		template <typename T, typename ...Args>
		MM_TS void createGroup(Args... args, bool localCall=false, byte channel=0, IDeliveryTrace* trace=nullptr);
		MM_TS virtual void destroyGroup( u32 groupId ) = 0;

		MM_TS virtual ESendCallResult sendReliable( BinSerializer& bs, const IEndpoint* specific=nullptr, bool exclude=false, bool buffer=false, 
													bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr ) = 0;


		static void setLogSettings( bool logToFile, bool logToIde );
	};



	template <typename T, typename ...Args>
	MM_TS ESendCallResult INetwork::callRpc(Args... args, bool localCall, const IEndpoint* specific, 
											bool exclude, bool buffer, bool relay, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., *this, bs, localCall);
		return priv_send_rpc( *this, T::rpcName(), bs, specific, exclude, buffer, relay, channel, trace );
	}

	template <typename T, typename ...Args>
	MM_TS void INetwork::createGroup(Args... args, bool localCall, byte channel, IDeliveryTrace* trace)
	{
		auto& bs = priv_get_thread_serializer();
		T::rpc<Args...>(args..., *this, bs, localCall);
		priv_create_group( *this, T::rpcName(), bs, channel, trace );
	}
}
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


	// ---------- User Classes -------------------------------

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

		MM_TS virtual void registerRpc( u16 rpcType, const std::function<void (BinSerializer&, INetwork&, const IEndpoint*)>& cb ) = 0;
		MM_TS virtual void deregisterRpc( u16 rpcType ) = 0;
		MM_TS virtual void callRpc( u16 rpcType, const IEndpoint* specific, bool exclude, bool buffer ) = 0;

		template <typename T, typename ...Args> 
		MM_TS void callRpc( Args... args, bool localCall=false, const IEndpoint* specific=nullptr, bool exclude=false, bool buffer=false );

		MM_TS virtual void registerGroup( u16 groupType, const std::function<void (INetwork&, const IEndpoint&)>& cb ) = 0;
		MM_TS virtual void deregisterGroup( u16 groupType ) = 0;
		MM_TS virtual void createGroup( u16 groupType ) = 0;
		MM_TS virtual void destroyGroup( u32 groupId ) = 0;


		static void setLogSettings( bool logToFile, bool logToIde );

		template <typename T> MM_TS void registerType();
		template <typename T> MM_TS void deregisterType();
		template <typename T, typename ...Args> MM_TS T* createType(Args... args);
		void destroyType(u32 id) { destroyGroup(id); }
	};



	template <typename T, typename ...Args>
	MM_TS void INetwork::callRpc(Args... args, bool localCall, const IEndpoint* specific, bool exclude, bool buffer)
	{
		T::rpc<Args...>(args..., *this, get_thread_serializer(), localCall);
	}

	template <typename T>
	MM_TS void INetwork::registerType()
	{
		registerGroup( T::typeId(), T::remoteCreate );
	}

	template <typename T>
	MM_TS void INetwork::deregisterType()
	{
		deregisterGroup( T::typeId() );
	}

	template <typename T, typename ...Args>
	MM_TS T* INetwork::createType(Args... args)
	{
		T* t = new T(args...);
		createGroup( T::typeId() );
		return t;
	}
}
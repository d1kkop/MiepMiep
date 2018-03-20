#pragma once

#include "Variables.h"
#include "Rpc.h"
#include <string>


namespace MiepMiep
{
	// ---------- Enums -------------------------------

	enum class EPacketType : byte
	{
		RPC,
		UserOffsetStart
	};

	enum class EConnectResult : byte
	{
		Fine,
		Timedout,
		InvalidPassword,
		MaxConnectionsReached,
		AlreadyConnected,
		NotConnecting
	};

	enum class EDisconnectReason : byte
	{
		Closed,
		Kicked,
		Lost
	};

	enum class ERegisterServerResult : byte
	{
		Fine,
		ServerIsFull
	};

	enum class EJoinServerResult : byte
	{
		Fine,
		NoMatchesFound,
		InvalidPassword,
		TimedOut,
		MaxConnectionsReached,
		AlreadyConnected,
		LogicError
	};

	enum class EListenCallResult
	{
		Fine,
		SocketError
	};

	enum class ECreateGroupCallResult
	{
		Fine,
		NoLinksInNetwork,
		MultipleLinksWhileNoSenderSpecified
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


	class MM_DECLSPEC IConnectionListener
	{
	public:
		virtual void onConnectResult( const ILink& link, EConnectResult res ) { }
		virtual void onNewConnection( const ILink& link, const IAddress& remote ) { }
		virtual void onDisconnect( const ILink& link, EDisconnectReason reason, const IAddress& remote ) { }
		virtual void onOwnerChanged( const ILink& link, NetVar& variable, const IAddress* newOwner ) { }
	};

	class MM_DECLSPEC ISender
	{
	public:
		virtual u64 id() const=0;

		MM_TS sptr<ISender> to_ptr();
		MM_TS sptr<const ISender> to_ptr() const;
	};


	class MM_DECLSPEC IAddress
	{
	public:
		MM_TS static sptr<IAddress> createEmpty();
		MM_TS static sptr<IAddress> resolve( const std::string& name, u16 port, i32* errOut=nullptr );
		MM_TS static sptr<IAddress> fromIpAndPort( const std::string& ipAndPort, i32* errOut=nullptr );

		/*	Returns a 'static thread_local' buffer. Do not delete the returned buffer. */
		virtual const char* toIpAndPort() const=0;
		virtual sptr<IAddress> getCopy() const=0;
		virtual u16 port() const=0;

		bool operator< ( const IAddress& other ) const;
		bool operator==( const IAddress& other ) const;
		bool operator==( const sptr<IAddress>& other ) const { return *this == *other; }
		bool operator!=( const IAddress& other ) const { return !(*this == other); }
		bool operator!=( const sptr<IAddress>& other ) { return !(*this == *other); }

		virtual bool write( class BinSerializer& bs ) const=0;
		virtual bool read( class BinSerializer& bs )=0;

		sptr<IAddress> to_ptr();
		sptr<const IAddress> to_ptr() const;
	};

	class MM_DECLSPEC ILink
	{
	public:
		MM_TS virtual INetwork& network() const=0;
		MM_TS virtual const IAddress& destination() const=0;
		MM_TS virtual const IAddress& source() const=0;
		MM_TS virtual bool  isConnected() const=0;

		MM_TS sptr<ILink> to_ptr();
		MM_TS sptr<const ILink> to_ptr() const;
	};


	class MM_DECLSPEC INetwork
	{
	public:
		/*	If asyncCallbacks == true, all network events may be called from different threads!
			The default is false. When false, 'processEvents' must be called to handle network events. */
		MM_TS static  sptr<INetwork> create( bool allowAsyncCallbacks=false );

		MM_TS virtual void processEvents()=0;

		MM_TS virtual EListenCallResult startListen( u16 port, const std::string& pw="", u32 maxConnections=32 )=0;
		MM_TS virtual bool stopListen( u16 port )=0;

		MM_TS virtual void registerServer( const std::function<void( const ILink& link, bool )>& callback,
										   const IAddress& masterAddr, bool isP2p, const std::string& serverName, const std::string& pw="",
										   const std::string& type="", const MetaData& hostMd=MetaData(),
										   float initialRating=0, const MetaData& customFilterMd=MetaData() )=0;

		MM_TS virtual void joinServer( const std::function<void( const ILink& link, EJoinServerResult )>& callback,
									   const IAddress& masterAddr, const std::string& serverName, const std::string& pw="",
									   const std::string& type="", const MetaData& joinMd=MetaData(),
									   float initialRating=0, float minRating=0, float maxRating=FLT_MAX,
									   u32 minPlayers=0, u32 maxPlayers=UINT_MAX )=0;

		template <typename T, typename ...Args>
		MM_TS ESendCallResult callRpc( Args... args, bool localCall=false, const IAddress* specific=nullptr, bool exclude=false, bool buffer=false,
									   bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr, const ISender* sender=nullptr );

		template <typename T, typename ...Args>
		MM_TS ECreateGroupCallResult createGroup( Args... args, bool localCall=false, byte channel=0, IDeliveryTrace* trace=nullptr, const ISender* sender=nullptr );
		MM_TS virtual void destroyGroup( u32 groupId )=0;

		MM_TS virtual void addConnectionListener( IConnectionListener* connectionListener )=0;
		MM_TS virtual void removeConnectionListener( const IConnectionListener* connectionListener )=0;

		MM_TS virtual ESendCallResult sendReliable( const ISender& sender, byte id, const BinSerializer* serializers, u32 numSerializers=1, const IAddress* specific=nullptr,
													bool exclude=false, bool buffer=false, bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr )=0;


		MM_TS static void setLogSettings( bool logToFile=true, bool logToIde=true );
	};


	template <typename T, typename ...Args>
	MM_TS ESendCallResult INetwork::callRpc( Args... args, bool localCall, const IAddress* specific,
											 bool exclude, bool buffer, bool relay, byte channel, IDeliveryTrace* trace, const ISender* sender )
	{
		auto& bs=priv_get_thread_serializer();
		T::rpc<Args...>( args..., *this, bs, localCall );
		return priv_send_rpc( *this, T::rpcName(), bs, specific, exclude, buffer, relay, false, channel, trace, sender );
	}

	template <typename T, typename ...Args>
	MM_TS ECreateGroupCallResult INetwork::createGroup( Args... args, bool localCall, byte channel, IDeliveryTrace* trace, const ISender* sender )
	{
		auto& bs=priv_get_thread_serializer();
		T::rpc<Args...>( args..., *this, bs, localCall );
		return priv_create_group( *this, T::rpcName(), bs, channel, trace, sender );
	}
}
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
		NoMatchesFound
	};

	enum class EListenCallResult
	{
		Fine,
		// See log.
		SocketError
	};

	enum class ECreateGroupCallResult
	{
		Fine,
		NoLinksInNetwork
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


	class MM_DECLSPEC IAddress
	{
	public:
		MM_TS static sptr<IAddress> createEmpty();
		MM_TS static sptr<IAddress> resolve( const std::string& name, u16 port, i32* errOut=nullptr );
		MM_TS static sptr<IAddress> fromIpAndPort( const std::string& ipAndPort, i32* errOut=nullptr );

		/*	Returns a 'static thread_local' buffer. Do not delete the returned buffer. */
		MM_TSC virtual const char* toIpAndPort() const=0;
		MM_TSC virtual sptr<IAddress> getCopy() const=0;
		MM_TSC virtual u16 port() const=0;

		MM_TSC bool operator< ( const IAddress& other ) const;
		MM_TSC bool operator==( const IAddress& other ) const;
		MM_TSC bool operator!=( const IAddress& other ) const { return !(*this == other); }

		virtual bool write( class BinSerializer& bs ) const=0;
		virtual bool read( class BinSerializer& bs )=0;

		MM_TSC sptr<IAddress> to_ptr();
		MM_TSC sptr<const IAddress> to_ptr() const;
	};


	class MM_DECLSPEC ILink
	{
	public:
		MM_TS virtual INetwork& network() const=0;
		MM_TS virtual ISession& session() const=0;
		MM_TS virtual const IAddress& destination() const=0;
		MM_TS virtual const IAddress& source() const=0;
		MM_TS virtual bool  isConnected() const=0;
		

		MM_TS sptr<ILink> to_ptr();
		MM_TS sptr<const ILink> to_ptr() const;
	};


	class MM_DECLSPEC ISession
	{
		
	};


	class MM_DECLSPEC INetwork
	{
	public:
		/*	If asyncCallbacks == true, all network events may be called from different threads!
			The default is false. When false, 'processEvents' must be called to handle network events. */
		MM_TS static  sptr<INetwork> create( bool allowAsyncCallbacks=false );

		MM_TS virtual void processEvents()=0;

		MM_TS virtual EListenCallResult startListen( u16 port )=0;
		MM_TS virtual void stopListen( u16 port )=0;

		/*	Returns false if creation failed. This should only ever occur when all ports on the local machine are in use. */
		MM_TS virtual bool registerServer( const std::function<void( const ILink& link, bool )>& callback,
										   const IAddress& masterAddr, bool isP2p, bool isPrivate, float rating,
										   u32 maxClients, const std::string& name, const std::string& type, const std::string& password,
										   const MetaData& hostMd=MetaData(), const MetaData& customMatchmakingMd=MetaData() )=0;

		/*	Returns false if creation failed. This should only ever occur when all porst on the local machine are in use. */
		MM_TS virtual bool joinServer( const std::function<void( const ILink& link, EJoinServerResult )>& callback,
									   const IAddress& masterAddr, const std::string& name, const std::string& type,
									   float minRating, float maxRating, u32 minPlayers, u32 maxPlayers,
									   bool findP2p, bool findClientServer,
									   const MetaData& joinMd=MetaData(), const MetaData& customMatchmakingMd=MetaData() )=0;

		template <typename T, typename ...Args>
		MM_TS ESendCallResult callRpc( Args... args, const ISession* session, const IAddress* exclOrSpecific=nullptr, bool localCall=false, bool buffer=false,
									   bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr );

		template <typename T, typename ...Args>
		MM_TS ECreateGroupCallResult createGroup( Args&&... args, const ISession& session, bool localCall=false, byte channel=0, IDeliveryTrace* trace=nullptr );
		MM_TS virtual void destroyGroup( u32 groupId )=0;

		MM_TS virtual void addConnectionListener( IConnectionListener* connectionListener )=0;
		MM_TS virtual void removeConnectionListener( const IConnectionListener* connectionListener )=0;

		MM_TS virtual ESendCallResult sendReliable( byte id, const ISession* session, ILink* exclOrSpecific, const BinSerializer* serializers, u32 numSerializers=1,
													bool buffer=false, bool relay=false, byte channel=0, IDeliveryTrace* trace=nullptr )=0;

		MM_TS static void setLogSettings( bool logToFile=true, bool logToIde=true );
	};


	template <typename T, typename ...Args>
	MM_TS ESendCallResult INetwork::callRpc( Args... args, const ISession* session, const IAddress* exclOrSpecific, bool localCall,
											 bool buffer, bool relay, byte channel, IDeliveryTrace* trace )
	{
		auto& bs=priv_get_thread_serializer();
		T::rpc<Args...>( args..., *this, bs, localCall );
		return priv_send_rpc( *this, T::rpcName(), bs, session, exclude, buffer, relay, false, channel, trace );
	}

	template <typename T, typename ...Args>
	MM_TS ECreateGroupCallResult INetwork::createGroup( Args&&... args, const ISession& session, bool localCall, byte channel, IDeliveryTrace* trace )
	{
		auto& bs=priv_get_thread_serializer();
		T::rpc<Args...>( args..., *this, bs, localCall );
		return priv_create_group( *this, session, T::rpcName(), bs, channel, trace );
	}
}
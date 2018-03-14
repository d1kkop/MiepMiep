#pragma once

#include "Memory.h"
#include "Platform.h"


namespace MiepMiep
{
	enum class IPProto
	{
		Ipv4,
		Ipv6
	};

	enum class ESendResult
	{
		Succes,
		Error,
		SocketClosed
	};

	enum class ERecvResult
	{
		Succes,
		NoData,
		Error,
		SocketClosed
	};

	struct SocketOptions
	{
		SocketOptions() :
			m_ReuseAddr(false),
			m_DontFragment(true)
		{
		}

		bool m_ReuseAddr;
		bool m_DontFragment;
	};


	class ISocket
	{
	protected:
		ISocket();

	public:
		static sptr<ISocket> create();
		virtual ~ISocket();

		bool operator< (const ISocket& right) const;
		bool operator==(const ISocket& right) const;
		bool operator!=(const ISocket& right) const { return !(*this==right); }

		// Interface
		virtual bool open(IPProto ipProto = IPProto::Ipv4, const SocketOptions& options=SocketOptions(), i32* err=nullptr) = 0;
		virtual bool bind(u16 port, i32* err=nullptr) = 0;
		virtual void close() = 0;
		virtual bool less(const ISocket& other) const = 0;
		virtual bool equal(const ISocket& other) const = 0;
		virtual ESendResult send( const class Endpoint& endPoint, const byte* data, u32 len, i32* err=nullptr ) const = 0;
		virtual ERecvResult recv( byte* buff, u32& rawSize, class Endpoint& endpointOut, i32* err=nullptr ) const = 0; // buffSize in, received size out

		// Shared
		bool isOpen() const  { return m_Open; }
		bool isBound() const { return m_Bound; }
		bool isBlocking() const { return m_Blocking; }
		IPProto getIpProtocol() const { return m_IpProto; }

		virtual sptr<ISocket> to_ptr() = 0;
		virtual sptr<const ISocket> to_ptr() const = 0;

	protected:
		bool m_Open;
		bool m_Bound;
		bool m_Blocking;
		IPProto m_IpProto;
	};


#if MM_SDLSOCKET
	class SDLSocket: public ISocket
	{
	public:
		SDLSocket();
		~SDLSocket() override;

		// ISocket
		virtual bool open(IPProto ipProto, bool reuseAddr) override;
		virtual bool bind(u16_t port) override;
		virtual bool close() override;
		virtual ESendResult send( const struct EndPoint& endPoint, const i8_t* data, i32_t len) const override;
		virtual ERecvResult recv( i8_t* buff, i32_t& rawSize, struct EndPoint& endPoint ) const override;

	protected:
		UDPsocket m_Socket;
		SDLNet_SocketSet m_SocketSet;
	};

#elif MM_WIN32SOCKET
	class BSDSocket: public ISocket, public ITraceable
	{
	public:
		BSDSocket();

		bool open(IPProto ipProto, const SocketOptions& options, i32* err) override;
		bool bind(u16 port, i32* err) override;
		void close() override;
		bool less(const ISocket& other) const override;
		bool equal(const ISocket& other) const override;
		ESendResult send( const class Endpoint& endPoint, const byte* data, u32 len, i32* err) const override;
		ERecvResult recv( byte* buff, u32& rawSize, class Endpoint& endPoint, i32* err ) const override;

		sptr<ISocket> to_ptr() override;
		sptr<const ISocket> to_ptr() const override;

		SOCKET getSock() const  { return m_Socket; }

	protected:
		SOCKET m_Socket;
	};
	#endif
}
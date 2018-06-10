#include "BinSerializer.h"
#include "Memory.h"
#include "Platform.h"
#include "Util.h"
#include "Endpoint.h"
#include <cassert>


namespace MiepMiep
{
	// ------------ BinSerializer ------------------------------------------------------------------------------

	BinSerializer::BinSerializer():
		m_DataPtr(nullptr)
	{
		reset();
	}

	BinSerializer::BinSerializer(const byte* streamIn, u32 buffSize, u32 writePos, bool copyData, bool ownsData):
		m_DataPtr(nullptr)
	{
		resetTo(streamIn, buffSize, writePos, copyData, ownsData);
	}

	BinSerializer::BinSerializer(const BinSerializer& other):
		BinSerializer()
	{
		write(other.data(), other.length());
	}

	BinSerializer::BinSerializer(BinSerializer&& other) noexcept
	{
		*this = move(other);
	}

	BinSerializer& BinSerializer::operator=(const BinSerializer& other)
	{
		write(other.data(), other.length());
		return *this;
	}

	BinSerializer& BinSerializer::operator=(BinSerializer&& other) noexcept
	{
		Platform::copy( this, &other, 1 );
		other.m_DataPtr = nullptr;
		return *this;
	}

	BinSerializer::~BinSerializer()
	{
		reset();
	}

	void BinSerializer::reset()
	{
		if ( m_Owns )
		{
			releaseN(m_DataPtr);
		}
		m_DataPtr  = nullptr;
		m_WritePos = 0;
		m_ReadPos  = 0;
		m_MaxSize  = 0;
		m_Owns = true;
	}

	void BinSerializer::resetTo(const byte* streamIn, u32 buffSize, u32 writePos, bool copyData, bool ownsData)
	{
		if ( m_Owns && !copyData )
		{
			Memory::doDeleteN(m_DataPtr);
			m_DataPtr = nullptr;
		}
		if ( !copyData )
		{
			m_DataPtr  = scc<byte*>( streamIn );
			m_WritePos = writePos;
			m_MaxSize  = buffSize;
		}
		else
		{
			growTo(buffSize);
			Platform::memCpy(m_DataPtr, buffSize, streamIn, buffSize);
			m_WritePos = writePos;
		}
		m_Owns = ownsData;
		m_ReadPos = 0;
		assert(m_WritePos>=0 && m_WritePos<=m_MaxSize && m_DataPtr!=nullptr);
	}

	bool BinSerializer::setRead(u32 r)
	{
		if ( r > m_WritePos ) return false;
		m_ReadPos = r;
		return true;
	}

	bool BinSerializer::setWrite(u32 w)
	{
		if ( w > m_MaxSize ) return false;
		m_WritePos = w;
		return true;
	}

	bool BinSerializer::moveRead(u32 r)
	{
		return setRead( m_ReadPos + r );
	}

	bool BinSerializer::moveWrite(u32 w)
	{
		return setWrite( m_WritePos + w );
	}

	bool BinSerializer::write8(byte b)
	{
		growTo(m_WritePos + 1);
		if ( m_WritePos + 1 > m_MaxSize ) return false;
		pr_data()[m_WritePos++] = b;
		return true;
	}

	bool BinSerializer::write16(u16 b)
	{
		growTo(m_WritePos + 2);
		if ( m_WritePos + 2 > m_MaxSize ) return false;
		*rc<u16*>(pr_data() + m_WritePos) = Util::htons(b);
		m_WritePos += 2;
		return true;
	}

	bool BinSerializer::write32(u32 b)
	{
		growTo(m_WritePos + 4);
		if ( m_WritePos + 4 > m_MaxSize ) return false;
		*rc<u32*>(pr_data() + m_WritePos) = Util::htonl(b);
		m_WritePos += 4;
		return true;
	}

	bool BinSerializer::write64(u64 b)
	{
		growTo(m_WritePos + 8);
		if ( m_WritePos + 8 > m_MaxSize ) return false;
		*rc<u64*>(pr_data() + m_WritePos) = Util::htonll(b);
		m_WritePos += 8;
		return true;
	}

	bool BinSerializer::read8(byte& b)
	{
		if ( m_ReadPos + 1 > m_WritePos ) return false;
		b = pr_data()[m_ReadPos++];
		return true;
	}

	bool BinSerializer::read16(u16& b)
	{
		if ( m_ReadPos + 2 > m_WritePos ) return false;
		b = Util::ntohs(*rc<u16*>(pr_data() + m_ReadPos));
		m_ReadPos += 2;
		return true;
	}

	bool BinSerializer::read32(u32& b)
	{
		if ( m_ReadPos + 4 > m_WritePos ) return false;
		b = Util::ntohl(*rc<u32*>(pr_data() + m_ReadPos));
		m_ReadPos += 4;
		return true;
	}

	bool BinSerializer::read64(u64& b)
	{
		if ( m_ReadPos + 8 > m_WritePos ) return false;
		b = Util::ntohll(*rc<u64*>(pr_data() + m_ReadPos));
		m_ReadPos += 8;
		return true;
	}

	void BinSerializer::copyAsRaw(byte*& ptr, u32& len)
	{
		len = m_WritePos;
		ptr = new byte[len];
		Platform::memCpy(ptr, len, m_DataPtr, len);
	}

	bool BinSerializer::read(byte* b, u32 buffSize)
	{
		if ( m_ReadPos + buffSize > m_WritePos ) return false;
		Platform::copy(b, data()+m_ReadPos, buffSize);
		m_ReadPos += buffSize;
		return true;
	}

	bool BinSerializer::write(const byte* b, u32 buffSize)
	{
		growTo(m_WritePos + buffSize);
		if ( m_WritePos + buffSize > m_MaxSize ) return false; // if data is not owned, it doenst grow
		Platform::copy(pr_data()+m_WritePos, b, buffSize);
		m_WritePos += buffSize;
		return true;
	}

	u32 BinSerializer::getMaxSize() const
	{
		return m_MaxSize;
	}

	u32 BinSerializer::getRead() const
	{
		return m_ReadPos;
	}

	u32 BinSerializer::getWrite() const
	{
		return m_WritePos;
	}

	u32 BinSerializer::length() const
	{
		return getWrite();
	}

	const byte* BinSerializer::data() const
	{
		return sc<const byte*>( pr_data() );
	}

	byte* BinSerializer::pr_data() const
	{
		return m_DataPtr;
	}

	void BinSerializer::growTo(u32 requiredSize)
	{
		if (!m_Owns || requiredSize <= m_MaxSize) return;
		m_MaxSize = u32(((requiredSize * 1.2f)/16) * 16 + 16);
		assert(m_MaxSize > requiredSize);
		byte* pNew = reserveN<byte>(MM_FL, m_MaxSize);
		Platform::copy(pNew, m_DataPtr, m_WritePos);
		releaseN(m_DataPtr);
		m_DataPtr = pNew;
	}


	// -- Templates

	template <typename T> bool BinSerializer::read(T& val)					{ return read(&val, sizeof(T)); }
	template <> bool BinSerializer::read(bool& b)							{ return read8((byte&)b); }
	template <> bool BinSerializer::read(byte& b)							{ return read8(b); }
	template <> bool BinSerializer::read(u16& b)							{ return read16(b); }
	template <> bool BinSerializer::read(u32& b)							{ return read32(b); }
	template <> bool BinSerializer::read(u64& b)							{ return read64(b); }
	template <> bool BinSerializer::read(char& b)							{ return read8((byte&)b); }
	template <> bool BinSerializer::read(i16& b)							{ return read16((u16&)b); }
	template <> bool BinSerializer::read(i32& b)							{ return read32((u32&)b); }
	template <> bool BinSerializer::read(i64& b)							{ return read64((u64&)b); }
	template <> bool BinSerializer::read(float& b)							{ return read32(sc<u32&>(*rc<u32*>(&b))); }
	template <> bool BinSerializer::read(double& b)							{ return read64(sc<u64&>(*rc<u64*>(&b))); }
	template <> bool BinSerializer::read(BinSerializer& other)				{ return other.write(data(), length()); }
	template <> bool BinSerializer::read(IAddress& b)						{ return b.read(*this); }
	template <> bool BinSerializer::read(sptr<IAddress>& b)
	{
		byte present;
		if ( !read(present) ) return false;
		if ( present )
		{
			b = IAddress::createEmpty();
			return b->read(*this);
		}
		return true; // yet empty
	}
	template <> bool BinSerializer::read(std::string& b) 
	{
		char buff[TempBuffSize];
		u16 k=0;
		do
		{
			if (k==TempBuffSize || m_ReadPos>=m_WritePos) return false;
			buff[k]=m_DataPtr[m_ReadPos++];
		} while (buff[k++]!='\0');
		b = buff;
		return true;
	}
	template <> bool BinSerializer::read(MetaData& b) 
	{ 
		u32 mlen;
		if (!read(mlen)) return false;
		std::string key, value;
		for (u16 i=0; i<mlen; ++i)
		{
			if ( !read(key) )   return false;
			if ( !read(value) ) return false;
			b.insert(make_pair(key, value));
		}
		return true;
	}

	template <typename T> bool BinSerializer::write(const T& val)			{ return write(&val, sizeof(T)); }
	template <> bool BinSerializer::write(const bool& b)					{ return write8((byte)b); }
	template <> bool BinSerializer::write(const byte& b)					{ return write8(b); }
	template <> bool BinSerializer::write(const u16& b)						{ return write16(b); }
	template <> bool BinSerializer::write(const u32& b)						{ return write32(b); }
	template <> bool BinSerializer::write(const u64& b)						{ return write64(b); }
	template <> bool BinSerializer::write(const char& b)					{ return write8(b); }
	template <> bool BinSerializer::write(const i16& b)						{ return write16(b); }
	template <> bool BinSerializer::write(const i32& b)						{ return write32(b); }
	template <> bool BinSerializer::write(const i64& b)						{ return write64((i64&)b); }
	template <> bool BinSerializer::write(const float& b)					{ return write32(sc<const u32&>(*rc<const u32*>(&b))); }
	template <> bool BinSerializer::write(const double& b)					{ return write64(sc<const u64&>(*rc<const u64*>(&b))); }
	template <> bool BinSerializer::write(const BinSerializer& other)		{ return write(other.data(), other.length()); }
	template <> bool BinSerializer::write(const IAddress& b)				{ return b.write(*this); }
	template <> bool BinSerializer::write(const sptr<IAddress>& b)			{ return write(const_pointer_cast<const IAddress>(b)); }
	template <> bool BinSerializer::write(const sptr<const IAddress>& b)
	{
		if (!b) return write(false);
		if (!write(true)) return false;
		return b->write(*this);
	}

	template <> bool BinSerializer::write(const std::string& b)
	{
		if ( b.length() > TempBuffSize-1 ) return false;
		return write(rc<const byte*>(b.c_str()), (u16)b.length()+1);
	}
	template <> bool BinSerializer::write(const MetaData& b)
	{
		if ( b.size() > UINT32_MAX ) return false;
		if ( !write<u32>((u32)b.size()) ) return false;
		for ( auto& kvp : b ) 
		{
			if ( !write(kvp.first) )  return false;
			if ( !write(kvp.second) ) return false;
		}
		return true;
	}
}
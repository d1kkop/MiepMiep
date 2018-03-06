#include "BinSerializer.h"
#include "Memory.h"
#include "Platform.h"
#include "Util.h"
#include <cassert>


namespace MiepMiep
{
	// ------------ BinSerializer ------------------------------------------------------------------------------

	BinSerializer::BinSerializer():
		m_Owns(false),
		m_OrigPtr(nullptr),
		m_OrigSize(0)
	{
		reset();
	}

	BinSerializer::BinSerializer(const BinSerializer& other)
	{
		reset();
		write(other.data(), other.length());
	}

	BinSerializer& BinSerializer::operator==(const BinSerializer& other)
	{
		reset();
		write(other.data(), other.length());
		return *this;
	}

	BinSerializer::~BinSerializer()
	{
		reset();
		Memory::doDeleteN(m_DataPtr);
	}

	void BinSerializer::reset()
	{
		if (!m_Owns) 
		{
			m_MaxSize = m_OrigSize;
			m_DataPtr = m_OrigPtr;
		}
		m_WritePos = 0;
		m_ReadPos  = 0;
		m_Owns = true;
	}

	void BinSerializer::resetTo(const byte* streamIn, u32 buffSize, u32 writePos)
	{
		if ( m_Owns )
		{
			m_OrigPtr  = m_DataPtr;
			m_OrigSize = m_MaxSize;
		}
		m_DataPtr  = (byte*) streamIn;
		m_WritePos = writePos;
		m_MaxSize = buffSize;
		m_ReadPos = 0;
		m_Owns = false;
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
		*(u16*)(pr_data() + m_WritePos) = Util::htons(b);
		m_WritePos += 2;
		return true;
	}

	bool BinSerializer::write32(u32 b)
	{
		growTo(m_WritePos + 4);
		if ( m_WritePos + 4 > m_MaxSize ) return false;
		*(u32*)(pr_data() + m_WritePos) = Util::htonl(b);
		m_WritePos += 4;
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
		b = Util::ntohs(*(u16*)(pr_data() + m_ReadPos));
		m_ReadPos += 2;
		return true;
	}

	bool BinSerializer::read32(u32& b)
	{
		if ( m_ReadPos + 4 > m_WritePos ) return false;
		b = Util::ntohl(*(u32*)(pr_data() + m_ReadPos));
		m_ReadPos += 4;
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

	byte* BinSerializer::pr_data() const
	{
		return m_DataPtr;
	}

	void BinSerializer::growTo(u32 requiredSize)
	{
		if (!m_Owns || requiredSize <= m_MaxSize) return;
		assert(requiredSize > 0);
		m_MaxSize = u32(((requiredSize * 1.2f)/16) * 16 + 16);
		assert(m_MaxSize > requiredSize);
		byte* pNew = reserveN<byte>(MM_FL, m_MaxSize);
		Platform::copy(pNew, m_DataPtr, m_WritePos);
		releaseN(m_DataPtr);
		m_DataPtr = pNew;
	}
}
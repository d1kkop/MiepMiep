#pragma once

#include "Types.h"
#include <map>


namespace MiepMiep
{
	constexpr u32 TempBuffSize = 4096;


	class MM_DECLSPEC BinSerializer
	{
	public:
		BinSerializer();
		BinSerializer(const BinSerializer& other);
		BinSerializer(BinSerializer&& other) noexcept;
		BinSerializer& operator=(const BinSerializer& other);
		BinSerializer& operator=(BinSerializer&& other) noexcept;
		~BinSerializer();

		void reset();
		void resetTo(const byte* streamIn, u32 buffSize, u32 writePos);
		void copyAsRaw(byte*& ptr, u32& len);

		bool setRead(u32 r);
		bool setWrite(u32 w);
		bool moveRead(u32 r);
		bool moveWrite(u32 w);
		bool read(byte* b, u32 buffSize);
		bool write(const byte* b, u32 buffSize);

		// --- Getters ----

		u32 getMaxSize() const		{ return m_MaxSize; }
		u32 getRead() const			{ return m_ReadPos; }
		u32 getWrite() const		{ return m_WritePos; }
		u32 length() const			{ return getWrite(); }
		const byte* data() const	{ return pr_data(); }

		// --- Templates ----

		template <typename T> bool write(const T& val)			{ return write(&val, sizeof(T)); }
		template <> bool write(const bool& b)					{ return write8((byte)b); }
		template <> bool write(const byte& b)					{ return write8(b); }
		template <> bool write(const u16& b)					{ return write16(b); }
		template <> bool write(const u32& b)					{ return write32(b); }
		template <> bool write(const u64& b)					{ return write64(b); }
		template <> bool write(const char& b)					{ return write8(b); }
		template <> bool write(const i16& b)					{ return write16(b); }
		template <> bool write(const i32& b)					{ return write32(b); }
		template <> bool write(const i64& b)					{ return write64((i64&)b); }
		template <> bool write(const BinSerializer& other)		{ return write(other.data(), other.length()); }
	//	template <> bool write(const IEndpoint& b)				{ return b.write(*this); }
		template <> bool write(const std::string& b)
		{
			if ( b.length() > TempBuffSize-1 ) return false;
			return write((const byte*)b.c_str(), (u16)b.length()+1);
		}
		template <> bool write(const MetaData& b)
		{
			if ( b.size() > UINT32_MAX ) return false;
			if ( !write<u32>((u32)b.size()) ) return false;
			for ( auto& kvp : b ) 
			{
				if ( !write(kvp.first) ) return false;
				if ( !write(kvp.second) ) return false;
			}
			return true;
		}
		

		template <typename T> bool read(T& val)					{ return read(&val, sizeof(T)); }
		template <> bool read(bool& b)							{ return read8((byte&)b); }
		template <> bool read(byte& b)							{ return read8(b); }
		template <> bool read(u16& b)							{ return read16(b); }
		template <> bool read(u32& b)							{ return read32(b); }
		template <> bool read(u64& b)							{ return read64(b); }
		template <> bool read(char& b)							{ return read8((byte&)b); }
		template <> bool read(i16& b)							{ return read16((u16&)b); }
		template <> bool read(i32& b)							{ return read32((u32&)b); }
		template <> bool read(i64& b)							{ return read64((u64&)b); }
		template <> bool read(BinSerializer& other)				{ return other.write(data(), length()); }
	//	template <> bool read(IEndpoint& b)						{ return b.read(*this); }
		template <> bool read(std::string& b) 
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
		template <> bool read(MetaData& b) 
		{ 
			u32 mlen;
			if (!read(mlen)) return false;
			std::string key, value;
			for (u16 i=0; i<mlen; i++)
			{
				if ( !read(key) )   return false;
				if ( !read(value) ) return false;
				b.insert(make_pair(key, value));
			}
			return true;
		}

		template <typename T> bool readOrWrite(T& t, bool _write)
		{
			return _write?write(t):read(t);
		}

	private:
		bool write8(byte b);
		bool write16(u16 b);
		bool write32(u32 b);
		bool write64(u64 b);
		bool read8(byte& b);
		bool read16(u16& b);
		bool read32(u32& b);
		bool read64(u64& b);
		byte* pr_data() const;
		void growTo(u32 maxSize);

	private:
		byte* m_DataPtr, * m_OrigPtr;
		u32   m_WritePos;
		u32   m_ReadPos;
		u32   m_MaxSize, m_OrigSize;
		bool  m_Owns;
	};
}
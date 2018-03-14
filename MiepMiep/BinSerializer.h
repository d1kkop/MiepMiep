#pragma once

#include "Types.h"


namespace MiepMiep
{
	constexpr u32 TempBuffSize = 4096;


	class MM_DECLSPEC BinSerializer
	{
	public:
		BinSerializer();
		BinSerializer(const byte* streamIn, u32 buffSize, u32 writePos, bool copyData=false, bool ownsData=false);
		BinSerializer(const BinSerializer& other);
		BinSerializer(BinSerializer&& other) noexcept;
		BinSerializer& operator=(const BinSerializer& other);
		BinSerializer& operator=(BinSerializer&& other) noexcept;
		~BinSerializer();

		void reset();
		void resetTo(const byte* streamIn, u32 buffSize, u32 writePos, bool copyData=false, bool ownsData=false);
		void copyAsRaw(byte*& ptr, u32& len);

		bool setRead(u32 r);
		bool setWrite(u32 w);
		bool moveRead(u32 r);
		bool moveWrite(u32 w);
		bool read(byte* b, u32 buffSize);
		bool write(const byte* b, u32 buffSize);

		// --- Getters ----

		u32 getMaxSize() const;
		u32 getRead() const;
		u32 getWrite() const;
		u32 length() const;
		const byte* data() const;

		// --- Templates ----

		template <typename T> bool write(const T& val);
		template <> bool write(const bool& b);
		template <> bool write(const byte& b);
		template <> bool write(const u16& b);
		template <> bool write(const u32& b);
		template <> bool write(const u64& b);
		template <> bool write(const char& b);
		template <> bool write(const i16& b);
		template <> bool write(const i32& b);
		template <> bool write(const i64& b);
		template <> bool write(const BinSerializer& other);
		template <> bool write(const IEndpoint& b);
		template <> bool write(const sptr<IEndpoint>& b);
		template <> bool write(const sptr<const IEndpoint>& b);
		template <> bool write(const std::string& b);
		template <> bool write(const MetaData& b);
		
		template <typename T> bool read(T& val);
		template <> bool read(bool& b);
		template <> bool read(byte& b);
		template <> bool read(u16& b);
		template <> bool read(u32& b);
		template <> bool read(u64& b);
		template <> bool read(char& b);
		template <> bool read(i16& b);
		template <> bool read(i32& b);
		template <> bool read(i64& b);
		template <> bool read(BinSerializer& other);
		template <> bool read(IEndpoint& b);
		template <> bool read(sptr<IEndpoint>& b);
		template <> bool read(std::string& b);
		template <> bool read(MetaData& b);

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
		byte* m_DataPtr;
		u32   m_WritePos;
		u32   m_ReadPos;
		u32   m_MaxSize;
		bool  m_Owns;
	};
}
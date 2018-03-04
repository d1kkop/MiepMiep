#pragma once

#include "Types.h"


namespace MiepMiep
{
	class Util
	{
	public:
		template <typename T>
		static T min(T a, T b) { return a < b ? a : b; }

		template <typename T>
		static T max(T a, T b) { return a > b ? a : b; }

		static u32 htonl( u32 val );
		static u16 htons( u16 val );
		static u32 ntohl( u32 val ) { return htonl(val); }
		static u16 ntohs( u16 val ) { return htons(val); }
	};
}
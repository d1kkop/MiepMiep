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

		static u64 htonll( u64 val );
		static u32 htonl( u32 val );
		static u16 htons( u16 val );
		static u64 ntohll( u64 val ) { return htonll(val); }
		static u32 ntohl( u32 val ) { return htonl(val); }
		static u16 ntohs( u16 val ) { return htons(val); }

		template <typename S, typename Pred>
		static void cluster( S s, u32 clusterSize, const Pred& pred );

		static u64 abs_time();
		static u32 rand();
	};



	template <typename S, typename Pred>
	void Util::cluster(S s, u32 clusterSize, const Pred& pred)
	{
		u32 off = 0;
		while (true)
		{
			if ( s > clusterSize ) 
			{
				pred( off, off + clusterSize );
				off += clusterSize;
			}
			else
			{
				pred( off, off + s );
				break;
			}
		}
	}

}
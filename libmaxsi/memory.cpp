/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	memory.cpp
	Useful functions for manipulating and handling memory.

******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/error.h>
#ifndef SORTIX_KERNEL
#include <libmaxsi/syscall.h>
#endif

namespace Maxsi
{
	namespace Memory
	{
#ifndef SORTIX_KERNEL
		DEFN_SYSCALL2(int, SysMemStat, SYSCALL_MEMSTAT, size_t*, size_t*);
		DEFN_SYSCALL1(void*, SysSbrk, SYSCALL_SBRK, intptr_t);
		DEFN_SYSCALL0(size_t, SysGetPageSize, SYSCALL_GET_PAGE_SIZE);

		extern "C" int memstat(size_t* memused, size_t* memtotal)
		{
			return SysMemStat(memused, memtotal);
		}

		extern "C" void* sbrk(intptr_t increment)
		{
			return SysSbrk(increment);
		}

		extern "C" size_t getpagesize(void)
		{
			return SysGetPageSize();
		}
#endif

		DUAL_FUNCTION(void*, memcpy, Copy, (void* Dest, const void* Src, size_t Length))
		{
			char* D = (char*) Dest; const char* S = (const char*) Src;

			for ( size_t I = 0; I < Length; I++ )
			{
				D[I] = S[I];
			}

			return Dest;
		}

		DUAL_FUNCTION(void*, memset, Set, (void* Dest, int Value, size_t Length))
		{
			byte* D = (byte*) Dest;

			for ( size_t I = 0; I < Length; I++ )
			{
				D[I] = Value & 0xFF;
			}

			return Dest;
		}

		DUAL_FUNCTION(int, memcmp, Compare, (const void* a, const void* b, size_t size))
		{
			const byte* buf1 = (const byte*) a;
			const byte* buf2 = (const byte*) b;
			for ( size_t i = 0; i < size; i++ )
			{
				if ( buf1[i] != buf2[i] ) { return (int)(buf1[i]) - (int)(buf2[i]); }
			}
			return 0;
		}

		DUAL_FUNCTION(void*, memchr, Seek, (const void* s, int c, size_t size))
		{
			const byte* buf = (const byte*) s;
			for ( size_t i = 0; i < size; i++ )
			{
				if ( buf[i] == c ) { return (void*) (buf + i); }
			}
			return NULL;	
		}
	}
}

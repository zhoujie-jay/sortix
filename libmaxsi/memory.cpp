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
#ifndef SORTIX_KERNEL
#include <sys/syscall.h>
#endif
#include <string.h>

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

		// TODO: This is a hacky implementation!
		extern "C" void* memmove(void* _dest, const void* _src, size_t n)
		{
			uint8_t* dest = (uint8_t*) _dest;
			const uint8_t* src = (const uint8_t*) _src;
			uint8_t* tmp = new uint8_t[n];
			memcpy(tmp, src, n);
			memcpy(dest, tmp, n);
			delete[] tmp;
			return _dest;
		}
#endif

		extern "C" void* memcpy_aligned(unsigned long* dest,
		                                const unsigned long* src,
		                                size_t length)
		{
			size_t numcopies = length / sizeof(unsigned long);
			for ( size_t i = 0; i < numcopies; i++ )
			{
				dest[i] = src[i];
			}
			return dest;
		}

		static inline bool IsWordAligned(uintptr_t addr)
		{
			const size_t WORDSIZE = sizeof(unsigned long);
			return (addr / WORDSIZE * WORDSIZE) == addr;
		}

		DUAL_FUNCTION(void*, memcpy, Copy, (void* destptr, const void* srcptr, size_t length))
		{
			if ( IsWordAligned((uintptr_t) destptr) &&
			     IsWordAligned((uintptr_t) srcptr) &&
			     IsWordAligned(length) )
			{
				unsigned long* dest = (unsigned long*) destptr;
				const unsigned long* src = (const unsigned long*) srcptr;
				return memcpy_aligned(dest, src, length);
			}
			uint8_t* dest = (uint8_t*) destptr;
			const uint8_t* src = (const uint8_t*) srcptr;
			for ( size_t i = 0; i < length; i += sizeof(uint8_t) )
			{
				dest[i] = src[i];
			}
			return dest;
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

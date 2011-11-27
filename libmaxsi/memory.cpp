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

#include "platform.h"
#include "memory.h"
#include "error.h"
#ifndef SORTIX_KERNEL
#include "syscall.h"
#endif

namespace Maxsi
{
	namespace Memory
	{
#ifndef SORTIX_KERNEL
		DEFN_SYSCALL2(int, SysMemStat, 32, size_t*, size_t*);

		extern "C" int memstat(size_t* memused, size_t* memtotal)
		{
			return SysMemStat(memused, memtotal);
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
	}
}

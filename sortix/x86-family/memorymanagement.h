/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	memorymanagement.h
	Handles memory for the x86 family of architectures.

******************************************************************************/

#ifndef SORTIX_X86_FAMILY_MEMORYMANAGEMENT_H
#define SORTIX_X86_FAMILY_MEMORYMANAGEMENT_H

namespace Sortix
{
	struct PML
	{
		addr_t entry[4096 / sizeof(addr_t)];
	};

	namespace Memory
	{
		const addr_t PML_PRESENT    = (1<<0);
		const addr_t PML_WRITABLE   = (1<<1);
		const addr_t PML_USERSPACE  = (1<<2);
		const addr_t PML_AVAILABLE1 = (1<<9);
		const addr_t PML_AVAILABLE2 = (1<<10);
		const addr_t PML_AVAILABLE3 = (1<<11);
		const addr_t PML_FORK       = PML_AVAILABLE1;
		const addr_t PML_FLAGS      = (0xFFFUL); // Bits used for the flags.
		const addr_t PML_ADDRESS    = (~0xFFFUL); // Bits used for the address.
	}
}

#ifdef PLATFORM_X86
#include "../x86/memorymanagement.h"
#endif

#ifdef PLATFORM_X64
#include "../x64/memorymanagement.h"
#endif

#endif

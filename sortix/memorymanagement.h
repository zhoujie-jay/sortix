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
	Functions that allow modification of virtual memory.

******************************************************************************/

#ifndef SORTIX_MEMORYMANAGEMENT_H
#define SORTIX_MEMORYMANAGEMENT_H

// Forward declarations.
typedef struct multiboot_info multiboot_info_t;

namespace Sortix
{
	namespace Page
	{
		addr_t Get();
		void Put(addr_t page);

		// Rounds a memory address down to nearest page.
		inline addr_t AlignDown(addr_t page) { return page & ~(0xFFFUL); }

		// Rounds a memory address up to nearest page.
		inline addr_t AlignUp(addr_t page) { return AlignDown(page + 0xFFFUL); }
	}

	namespace Memory
	{
		void Init(multiboot_info_t* bootinfo);
		void InvalidatePage(addr_t addr);
		void Flush();
		addr_t Fork();
		addr_t SwitchAddressSpace(addr_t addrspace);
		void DestroyAddressSpace();
		bool MapRangeKernel(addr_t where, size_t bytes);
		void UnmapRangeKernel(addr_t where, size_t bytes);
		bool MapRangeUser(addr_t where, size_t bytes);
		void UnmapRangeUser(addr_t where, size_t bytes);
		bool MapKernel(addr_t physical, addr_t mapto);
		bool MapUser(addr_t physical, addr_t mapto);
		addr_t UnmapKernel(addr_t mapto);
		addr_t UnmapUser(addr_t mapto);
		void Statistics(size_t* amountused, size_t* totalmem);

#if defined(PLATFORM_X86)
		const addr_t HEAPLOWER = 0x80000000UL;
		const addr_t HEAPUPPER = 0xFF400000UL;
#elif defined(PLATFORM_X64)
		const addr_t HEAPLOWER = 0xFFFF800000000000UL;
		const addr_t HEAPUPPER = 0xFFFFFE8000000000UL;
#endif
	}
}

#endif


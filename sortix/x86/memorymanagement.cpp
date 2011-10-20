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

	memorymanagement.cpp
	Handles memory for the x86 architecture.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "multiboot.h"
#include "panic.h"
#include "../memorymanagement.h"
#include "x86-family/memorymanagement.h"

namespace Sortix
{
	namespace Page
	{
		extern size_t stackused;
		extern size_t stacklength;
	}

	namespace Memory
	{
		extern addr_t currentdir;

		void InitCPU()
		{
			PML* const BOOTPML2 = (PML* const) 0x01000UL;
			PML* const BOOTPML1 = (PML* const) 0x02000UL;
			PML* const FORKPML1 = (PML* const) 0x03000UL;
			PML* const IDENPML1 = (PML* const) 0x04000UL;

			// Initialize the memory structures with zeroes.
			Maxsi::Memory::Set((PML* const) 0x01000UL, 0, 0x6000UL);

			// Identity map the first 4 MiB.
			addr_t flags = PML_PRESENT | PML_WRITABLE;

			BOOTPML2->entry[0] = ((addr_t) IDENPML1) | flags;

			for ( size_t i = 0; i < ENTRIES; i++ )
			{
				IDENPML1->entry[i] = (i * 4096UL) | flags;
			}

			// Next order of business is to map the virtual memory structures
			// to the pre-defined locations in the virtual address space.

			// Fractal map the PML1s.
			BOOTPML2->entry[1023] = (addr_t) BOOTPML2 | flags;

			// Fractal map the PML2s.
			BOOTPML2->entry[1022] = (addr_t) BOOTPML1 | flags | PML_FORK;
			BOOTPML1->entry[1023] = (addr_t) BOOTPML2 | flags;

			// Add some predefined room for forking address spaces.
			BOOTPML1->entry[0] = (addr_t) FORKPML1 | flags | PML_FORK;

			// The virtual memory structures are now available on the predefined
			// locations. This means the virtual memory code is bootstrapped. Of
			// course, we still have no physical page allocator, so that's the
			// next step.

			PML* const PHYSPML1 = (PML* const) 0x05000UL;
			PML* const PHYSPML0 = (PML* const) 0x06000UL;

			BOOTPML2->entry[1021] = (addr_t) PHYSPML1 | flags;
			PHYSPML1->entry[0]    = (addr_t) PHYSPML0 | flags;

			// Alright, enable virtual memory!
			SwitchAddressSpace((addr_t) BOOTPML2);

			size_t cr0;
			asm volatile("mov %%cr0, %0": "=r"(cr0));
			cr0 |= 0x80000000UL; /* Enable paging! */
			asm volatile("mov %0, %%cr0":: "r"(cr0));

			Page::stackused = 0;
			Page::stacklength = 4096UL / sizeof(addr_t);

			// The physical memory allocator should now be ready for use. Next
			// up, the calling function will fill up the physical allocator with
			// plenty of nice physical pages. (see Page::InitPushRegion)
		}
	}
}

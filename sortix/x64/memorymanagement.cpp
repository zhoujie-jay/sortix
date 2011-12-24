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
	Handles memory for the x64 architecture.

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
			// The x64 boot code already set up virtual memory and identity
			// mapped the first 2 MiB. This code finishes the job such that
			// virtual memory is fully usable and manageable.

			// boot.s already initialized everything from 0x1000UL to 0xE000UL
			// to zeroes. Since these structures are already used, doing it here
			// will be very dangerous.

			PML* const BOOTPML4 = (PML* const) 0x21000UL;
			PML* const BOOTPML3 = (PML* const) 0x26000UL;
			PML* const BOOTPML2 = (PML* const) 0x27000UL;
			PML* const BOOTPML1 = (PML* const) 0x28000UL;

			// First order of business is to map the virtual memory structures
			// to the pre-defined locations in the virtual address space.
			addr_t flags = PML_PRESENT | PML_WRITABLE;

			// Fractal map the PML1s.
			BOOTPML4->entry[511] = (addr_t) BOOTPML4 | flags;

			// Fractal map the PML2s.
			BOOTPML4->entry[510] = (addr_t) BOOTPML3 | flags | PML_FORK;
			BOOTPML3->entry[511] = (addr_t) BOOTPML4 | flags;

			// Fractal map the PML3s.
			BOOTPML3->entry[510] = (addr_t) BOOTPML2 | flags | PML_FORK;
			BOOTPML2->entry[511] = (addr_t) BOOTPML4 | flags;

			// Fractal map the PML4s.
			BOOTPML2->entry[510] = (addr_t) BOOTPML1 | flags | PML_FORK;
			BOOTPML1->entry[511] = (addr_t) BOOTPML4 | flags;

			// Add some predefined room for forking address spaces.
			PML* const FORKPML2 = (PML* const) 0x29000UL;
			PML* const FORKPML1 = (PML* const) 0x2A000UL;
			
			BOOTPML3->entry[0] = (addr_t) FORKPML2 | flags | PML_FORK;
			BOOTPML2->entry[0] = (addr_t) FORKPML1 | flags | PML_FORK;

			currentdir = (addr_t) BOOTPML4;

			// The virtual memory structures are now available on the predefined
			// locations. This means the virtual memory code is bootstrapped. Of
			// course, we still have no physical page allocator, so that's the
			// next step.

			PML* const PHYSPML3 = (PML* const) 0x2B000UL;
			PML* const PHYSPML2 = (PML* const) 0x2C000UL;
			PML* const PHYSPML1 = (PML* const) 0x2D000UL;
			PML* const PHYSPML0 = (PML* const) 0x2E000UL;

			BOOTPML4->entry[509] = (addr_t) PHYSPML3 | flags;
			PHYSPML3->entry[0]   = (addr_t) PHYSPML2 | flags;
			PHYSPML2->entry[0]   = (addr_t) PHYSPML1 | flags;
			PHYSPML1->entry[0]   = (addr_t) PHYSPML0 | flags;

			Page::stackused = 0;
			Page::stacklength = 4096UL / sizeof(addr_t);

			// The physical memory allocator should now be ready for use. Next
			// up, the calling function will fill up the physical allocator with
			// plenty of nice physical pages. (see Page::InitPushRegion)
		}

		// Please note that even if this function exists, you should still clean
		// up the address space of a process _before_ calling
		// DestroyAddressSpace. This is just a hack because it currently is
		// impossible to clean up PLM1's using the MM api!
		// ---
		// TODO: This function is duplicated in {x86,x64}/memorymanagement.cpp!
		// ---
		void RecursiveFreeUserspacePages(size_t level, size_t offset)
		{
			PML* pml = PMLS[level] + offset;
			for ( size_t i = 0; i < ENTRIES; i++ )
			{
				if ( !(pml->entry[i] & PML_PRESENT) ) { continue; }
				if ( !(pml->entry[i] & PML_USERSPACE) ) { continue; }
				if ( !(pml->entry[i] & PML_FORK) ) { continue; }
				if ( level > 1 ) { RecursiveFreeUserspacePages(level-1, offset * ENTRIES + i); }
				addr_t addr = pml->entry[i] & PML_ADDRESS;
				pml->entry[i] = 0;
				Page::Put(addr);
			}
		}

		void DestroyAddressSpace()
		{
			// First let's do the safe part. Garbage collect any PML1/0's left
			// behind by user-space. These are completely safe to delete.
			RecursiveFreeUserspacePages(TOPPMLLEVEL, 0);

			// TODO: Right now this just leaks memory.

			// Switch to the address space from when the world was originally
			// created. It should contain the kernel, the whole kernel, and
			// nothing but the kernel.
			PML* const BOOTPML4 = (PML* const) 0x21000UL;
			SwitchAddressSpace((addr_t) BOOTPML4);
		}

		const size_t KERNEL_STACK_SIZE = 256UL * 1024UL;
		const addr_t KERNEL_STACK_END = 0xFFFF800000001000UL;
		const addr_t KERNEL_STACK_START = KERNEL_STACK_END + KERNEL_STACK_SIZE;
		addr_t INITRD = KERNEL_STACK_START;
		size_t initrdsize = 0;
		const addr_t HEAPUPPER = 0xFFFFFE8000000000UL;

		addr_t GetInitRD()
		{
			return INITRD;
		}

		size_t GetInitRDSize()
		{
			return initrdsize;
		}

		void RegisterInitRDSize(size_t size)
		{
			initrdsize = size;
		}

		addr_t GetHeapLower()
		{
			return Page::AlignUp(INITRD + initrdsize);
		}

		addr_t GetHeapUpper()
		{
			return HEAPUPPER;
		}

		addr_t GetKernelStack()
		{
			return KERNEL_STACK_START;
		}

		size_t GetKernelStackSize()
		{
			return KERNEL_STACK_SIZE;
		}
	}
}

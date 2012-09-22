/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	memorymanagement.cpp
	Handles memory for the x86 architecture.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <string.h>
#include "multiboot.h"
#include <sortix/kernel/panic.h>
#include <sortix/kernel/memorymanagement.h>
#include "x86-family/memorymanagement.h"

namespace Sortix
{
	namespace Page
	{
		extern size_t stackused;
		extern size_t stacklength;
		void ExtendStack();
	}

	namespace Memory
	{
		extern addr_t currentdir;

		void InitCPU()
		{
			PML* const BOOTPML2 = (PML* const) 0x11000UL;
			PML* const BOOTPML1 = (PML* const) 0x12000UL;
			PML* const FORKPML1 = (PML* const) 0x13000UL;
			PML* const IDENPML1 = (PML* const) 0x14000UL;

			// Initialize the memory structures with zeroes.
			memset((PML* const) 0x11000UL, 0, 0x6000UL);

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
			BOOTPML1->entry[0] = 0; // (addr_t) FORKPML1 | flags | PML_FORK;

			// The virtual memory structures are now available on the predefined
			// locations. This means the virtual memory code is bootstrapped. Of
			// course, we still have no physical page allocator, so that's the
			// next step.

			PML* const PHYSPML1 = (PML* const) 0x15000UL;
			PML* const PHYSPML0 = (PML* const) 0x16000UL;

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
				addr_t entry = pml->entry[i];
				if ( !(entry & PML_PRESENT) ) { continue; }
				if ( !(entry & PML_USERSPACE) ) { continue; }
				if ( !(entry & PML_FORK) ) { continue; }
				if ( level > 1 ) { RecursiveFreeUserspacePages(level-1, offset * ENTRIES + i); }
				addr_t addr = pml->entry[i] & PML_ADDRESS;
				// No need to unmap the page, we just need to mark it as unused.
				Page::PutUnlocked(addr);
			}
		}

		void DestroyAddressSpace(addr_t fallback, void (*func)(addr_t, void*), void* user)
		{
			// Look up the last few entries used for the fractal mapping. These
			// cannot be unmapped as that would destroy the world. Instead, we
			// will remember them, switch to another adress space, and safely
			// mark them as unused. Also handling the forking related pages.
			addr_t fractal1 = PMLS[2]->entry[1022];
			addr_t dir = currentdir;

			// We want to free the pages, but we are still using them ourselves,
			// so lock the page allocation structure until we are done.
			Page::Lock();

			// In case any pages wasn't cleaned at this point.
#warning Page::Put calls may internally Page::Get and then reusing pages we are not done with just yet
			RecursiveFreeUserspacePages(TOPPMLLEVEL, 0);

			// Switch to the address space from when the world was originally
			// created. It should contain the kernel, the whole kernel, and
			// nothing but the kernel.
			PML* const BOOTPML2 = (PML* const) 0x11000UL;
			if ( !fallback )
				fallback = (addr_t) BOOTPML2;

			if ( func )
				func(fallback, user);
			else
				SwitchAddressSpace(fallback);

			// Ok, now we got marked everything left behind as unused, we can
			// now safely let another thread use the pages.
			Page::Unlock();

			// These are safe to free since we switched address space.
			Page::Put(fractal1 & PML_ADDRESS);
			Page::Put(dir & PML_ADDRESS);
		}

		const size_t KERNEL_STACK_SIZE = 256UL * 1024UL;
		const addr_t KERNEL_STACK_END = 0x80001000UL;
		const addr_t KERNEL_STACK_START = KERNEL_STACK_END + KERNEL_STACK_SIZE;
		const addr_t VIDEO_MEMORY = KERNEL_STACK_START;
		const size_t VIDEO_MEMORY_MAX_SIZE = 256UL * 1024UL * 1024UL;
		const addr_t INITRD = VIDEO_MEMORY + VIDEO_MEMORY_MAX_SIZE;
		size_t initrdsize = 0;
		const addr_t HEAPUPPER = 0xFF400000UL;

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

		addr_t GetVideoMemory()
		{
			return VIDEO_MEMORY;
		}

		size_t GetMaxVideoMemorySize()
		{
			return VIDEO_MEMORY_MAX_SIZE;
		}
	}
}

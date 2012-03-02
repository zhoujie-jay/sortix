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
		void ExtendStack();
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
			BOOTPML1->entry[0] = 0; // (addr_t) FORKPML1 | flags | PML_FORK;

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

			// Let's destroy the current address space! Oh wait. If we do that,
			// hell will break loose half-way when we start unmapping this piece
			// of code.
			// Instead, let's just mark the relevant pages as unused and switch
			// to another address space as fast as humanely possible. Any memory
			// allocation could potentially modify the current paging structures
			// and overwrite their contents causing a tripple-fault!

			// Make sure Page::Put does NOT cause any Page::Get's internally!
			const size_t NUM_PAGES = 2;
			if ( Page::stacklength - Page::stackused < NUM_PAGES ) { Page::ExtendStack(); }

			addr_t fractal1 = PMLS[2]->entry[1022];

			Page::Put(fractal1 & PML_ADDRESS);
			Page::Put(currentdir & PML_ADDRESS);

			// Switch to the address space from when the world was originally
			// created. It should contain the kernel, the whole kernel, and
			// nothing but the kernel.
			PML* const BOOTPML2 = (PML* const) 0x01000UL;
			SwitchAddressSpace((addr_t) BOOTPML2);
		}

		const size_t KERNEL_STACK_SIZE = 256UL * 1024UL;
		const addr_t KERNEL_STACK_END = 0x80001000UL;
		const addr_t KERNEL_STACK_START = KERNEL_STACK_END + KERNEL_STACK_SIZE;
		addr_t INITRD = KERNEL_STACK_START;
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
	}
}

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    x64/memorymanagement.cpp
    Handles memory for the x64 architecture.

*******************************************************************************/

#include <string.h>

#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>

#include "multiboot.h"
#include "x86-family/memorymanagement.h"

namespace Sortix {
namespace Page {

extern size_t stackused;
extern size_t stacklength;
void ExtendStack();

} // namespace Page
} // namespace Sortix

namespace Sortix {
namespace Memory {

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
		addr_t entry = pml->entry[i];
		if ( !(entry & PML_PRESENT) )
			continue;
		if ( !(entry & PML_USERSPACE) )
			continue;
		if ( !(entry & PML_FORK) )
			continue;
		if ( 1 < level )
			RecursiveFreeUserspacePages(level-1, offset * ENTRIES + i);
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
	addr_t fractal3 = (PMLS[4] + 0)->entry[510UL];
	addr_t fork2 = (PMLS[3] + 510UL)->entry[0];
	addr_t fractal2 = (PMLS[3] + 510UL)->entry[510];
	addr_t fork1 = (PMLS[2] + 510UL * 512UL + 510UL)->entry[0];
	addr_t fractal1 = (PMLS[2] + 510UL * 512UL + 510UL)->entry[510];
	addr_t dir = currentdir;

	// We want to free the pages, but we are still using them ourselves,
	// so lock the page allocation structure until we are done.
	Page::Lock();

	// In case any pages wasn't cleaned at this point.
	// TODO: Page::Put calls may internally Page::Get and then reusing pages we are not done with just yet
	RecursiveFreeUserspacePages(TOPPMLLEVEL, 0);

	// Switch to the address space from when the world was originally
	// created. It should contain the kernel, the whole kernel, and
	// nothing but the kernel.
	PML* const BOOTPML4 = (PML* const) 0x21000UL;
	if ( !fallback )
		fallback = (addr_t) BOOTPML4;

	if ( func )
		func(fallback, user);
	else
		SwitchAddressSpace(fallback);

	// Ok, now we got marked everything left behind as unused, we can
	// now safely let another thread use the pages.
	Page::Unlock();

	// These are safe to free since we switched address space.
	Page::Put(fractal3 & PML_ADDRESS);
	Page::Put(fractal2 & PML_ADDRESS);
	Page::Put(fractal1 & PML_ADDRESS);
	Page::Put(fork2 & PML_ADDRESS);
	Page::Put(fork1 & PML_ADDRESS);
	Page::Put(dir & PML_ADDRESS);
}

const size_t KERNEL_STACK_SIZE = 256UL * 1024UL;
const addr_t KERNEL_STACK_END = 0xFFFF800000001000UL;
const addr_t KERNEL_STACK_START = KERNEL_STACK_END + KERNEL_STACK_SIZE;

const addr_t VIRTUAL_AREA_LOWER = KERNEL_STACK_START;
const addr_t VIRTUAL_AREA_UPPER = 0xFFFFFE8000000000UL;

void GetKernelVirtualArea(addr_t* from, size_t* size)
{
	*from = KERNEL_STACK_END;
	*size = VIRTUAL_AREA_UPPER - VIRTUAL_AREA_LOWER;
}

void GetUserVirtualArea(uintptr_t* from, size_t* size)
{
	*from = 0x400000; // 4 MiB.
	*size = 0x800000000000 - *from; // 128 TiB - 4 MiB.
}

addr_t GetKernelStack()
{
	return KERNEL_STACK_START;
}

size_t GetKernelStackSize()
{
	return KERNEL_STACK_SIZE;
}

} // namespace Memory
} // namespace Sortix

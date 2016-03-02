/*
 * Copyright (c) 2011, 2012, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * x86/memorymanagement.cpp
 * Handles memory for the x86 architecture.
 */

#include <string.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/panic.h>

#include "x86-family/memorymanagement.h"

#include "multiboot.h"

namespace Sortix {
namespace Page {

extern size_t stackused;
extern size_t stacklength;
void ExtendStack();

} // namespace Page
} // namespace Sortix

namespace Sortix {
namespace Memory {

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
		Page::PutUnlocked(addr, PAGE_USAGE_PAGING_OVERHEAD);
	}
}

void DestroyAddressSpace(addr_t fallback)
{
	// Look up the last few entries used for the fractal mapping. These
	// cannot be unmapped as that would destroy the world. Instead, we
	// will remember them, switch to another adress space, and safely
	// mark them as unused. Also handling the forking related pages.
	addr_t fractal1 = PMLS[2]->entry[1022];
	addr_t dir = GetAddressSpace();

	// We want to free the pages, but we are still using them ourselves,
	// so lock the page allocation structure until we are done.
	Page::Lock();

	// In case any pages wasn't cleaned at this point.
	// TODO: Page::Put calls may internally Page::Get and then reusing pages we are not done with just yet
	RecursiveFreeUserspacePages(TOPPMLLEVEL, 0);

	SwitchAddressSpace(fallback);

	// Ok, now we got marked everything left behind as unused, we can
	// now safely let another thread use the pages.
	Page::Unlock();

	// These are safe to free since we switched address space.
	Page::Put(fractal1 & PML_ADDRESS, PAGE_USAGE_PAGING_OVERHEAD);
	Page::Put(dir & PML_ADDRESS, PAGE_USAGE_PAGING_OVERHEAD);
}

const size_t KERNEL_STACK_SIZE = 256UL * 1024UL;
const addr_t KERNEL_STACK_END = 0x80001000UL;
const addr_t KERNEL_STACK_START = KERNEL_STACK_END + KERNEL_STACK_SIZE;

const addr_t VIRTUAL_AREA_LOWER = KERNEL_STACK_START;
const addr_t VIRTUAL_AREA_UPPER = 0xFF400000UL;

void GetKernelVirtualArea(addr_t* from, size_t* size)
{
	*from = KERNEL_STACK_END;
	*size = VIRTUAL_AREA_UPPER - VIRTUAL_AREA_LOWER;
}

void GetUserVirtualArea(uintptr_t* from, size_t* size)
{
	*from = 0x400000; // 4 MiB.
	*size = 0x80000000 - *from; // 2 GiB - 4 MiB.
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

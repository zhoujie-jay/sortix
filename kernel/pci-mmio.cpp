/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    pci-mmio.cpp
    Functions for handling PCI devices.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sortix/mman.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/pci-mmio.h>

#if defined(__i386__) || defined(__x86_64__)
#include "x86-family/memorymanagement.h"
#endif

namespace Sortix {

bool MapPCIBAR(addralloc_t* allocation, pcibar_t bar, int flags)
{
	if ( !bar.is_mmio() )
		return errno = EINVAL, false;

	uint64_t phys_addr = bar.addr();
	uint64_t phys_size = bar.size();

	size_t addr_unalignment = phys_addr % Page::Size();
	if ( addr_unalignment )
	{
		phys_addr -= addr_unalignment;
		if ( (uint64_t) 0 - addr_unalignment <= phys_size )
			return errno = EOVERFLOW, false;
		phys_size += addr_unalignment;
	}

	size_t size_unalignment = phys_size % Page::Size();
	if ( size_unalignment )
	{
		phys_size -= size_unalignment;
		phys_size += Page::Size();
		if ( phys_size == 0 )
			return errno = EOVERFLOW, false;
	}

	if ( SIZE_MAX < phys_size )
		return errno = EOVERFLOW, false;

	size_t size = phys_size;

	if ( !AllocateKernelAddress(allocation, size) )
		return false;

	for ( size_t i = 0; i < size; i += Page::Size() )
	{
		bool failure = false;

		int prot = PROT_KWRITE | PROT_KREAD;
		uint64_t physaddr = bar.addr() + i;
		uintptr_t mapat = allocation->from + i;

		if ( sizeof(void*) <= 4 && 0x100000000 <= physaddr )
			errno = EOVERFLOW, failure = true;
#if defined(__i386__) || defined(__x86_64__)
		else if ( flags & MAP_PCI_BAR_WRITE_COMBINE )
		{
			const addr_t mtype = Memory::PAT_WC;
			if ( !Memory::MapPAT(physaddr, mapat, prot, mtype) )
				failure = true;
		}
#endif
		else
		{
			if ( !Memory::Map(physaddr, mapat, prot) )
				failure = true;
		}

		if ( failure )
		{
			for ( size_t n = 0; n < i; n += Page::Size() )
				Memory::Unmap(allocation->from + n);
			Memory::Flush();
			FreeKernelAddress(allocation);
			return false;
		}
	}

	Memory::Flush();

	allocation->from += addr_unalignment;
	allocation->size = bar.size();

	return true;
}

void UnmapPCIBar(addralloc_t* allocation)
{
	size_t unalignment = allocation->from % Page::Size();
	allocation->from = Page::AlignDown(allocation->from);
	allocation->size = Page::AlignUp(unalignment + allocation->size);
	for ( size_t i = 0; i < allocation->size; i += Page::Size() )
		Memory::Unmap(allocation->from + i);
	Memory::Flush();
	FreeKernelAddress(allocation);
	memset(allocation, 0, sizeof(*allocation));
}

} // namespace Sortix

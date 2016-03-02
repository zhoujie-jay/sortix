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
 * x86-family/memorymanagement.cpp
 * Handles memory for the x86 family of architectures.
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/mman.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/panic.h>
#include <sortix/kernel/pat.h>
#include <sortix/kernel/syscall.h>

#include "multiboot.h"
#include "memorymanagement.h"
#include "msr.h"

namespace Sortix {

extern size_t end;

} // namespace Sortix

namespace Sortix {
namespace Page {

void InitPushRegion(addr_t position, size_t length);
size_t pagesnotonstack = 0;
size_t stackused = 0;
size_t stackreserved = 0;
size_t stacklength = 4096 / sizeof(addr_t);
size_t totalmem = 0;
size_t page_usage_counts[PAGE_USAGE_NUM_KINDS];
kthread_mutex_t pagelock = KTHREAD_MUTEX_INITIALIZER;

} // namespace Page
} // namespace Sortix

namespace Sortix {
namespace Memory {

addr_t PAT2PMLFlags[PAT_NUM];

static bool CheckUsedRange(addr_t test,
                           addr_t from_unaligned,
                           size_t size_unaligned,
                           size_t* dist_ptr)
{
	addr_t from = Page::AlignDown(from_unaligned);
	size_unaligned += from_unaligned - from;
	size_t size = Page::AlignUp(size_unaligned);
	if ( from <= test && test < from + size )
		return *dist_ptr = from + size - test, true;
	if ( test < from && from - test < *dist_ptr )
		*dist_ptr = from - test;
	return false;
}

static bool CheckUsedString(addr_t test,
                            const char* string,
                            size_t* dist_ptr)
{
	size_t size = strlen(string) + 1;
	return CheckUsedRange(test, (addr_t) string, size, dist_ptr);
}

static bool CheckUsedRanges(multiboot_info_t* bootinfo,
                            addr_t test,
                            size_t* dist_ptr)
{
	addr_t kernel_end = (addr_t) &end;
	if ( CheckUsedRange(test, 0, kernel_end, dist_ptr) )
		return true;

	if ( CheckUsedRange(test, (addr_t) bootinfo, sizeof(*bootinfo), dist_ptr) )
		return true;

	const char* cmdline = (const char*) (uintptr_t) bootinfo->cmdline;
	if ( CheckUsedString(test, cmdline, dist_ptr) )
		return true;

	size_t mods_size = bootinfo->mods_count * sizeof(struct multiboot_mod_list);
	if ( CheckUsedRange(test, bootinfo->mods_addr, mods_size, dist_ptr) )
		return true;

	struct multiboot_mod_list* modules =
		(struct multiboot_mod_list*) (uintptr_t) bootinfo->mods_addr;
	for ( uint32_t i = 0; i < bootinfo->mods_count; i++ )
	{
		struct multiboot_mod_list* module = &modules[i];
		assert(module->mod_start <= module->mod_end);
		size_t mod_size = module->mod_end - module->mod_start;
		if ( CheckUsedRange(test, module->mod_start, mod_size, dist_ptr) )
			return true;
		const char* mod_cmdline = (const char*) (uintptr_t) module->cmdline;
		if ( CheckUsedString(test, mod_cmdline, dist_ptr) )
			return true;
	}

	if ( CheckUsedRange(test, bootinfo->mmap_addr, bootinfo->mmap_length,
	                      dist_ptr) )
		return true;

	return false;
}

void Init(multiboot_info_t* bootinfo)
{
	if ( !(bootinfo->flags & MULTIBOOT_INFO_MEM_MAP) )
		Panic("The memory map flag was't set in the multiboot structure.");

	// If supported, setup the Page Attribute Table feature that allows
	// us to control the memory type (caching) of memory more precisely.
	if ( IsPATSupported() )
	{
		InitializePAT();
		for ( addr_t i = 0; i < PAT_NUM; i++ )
			PAT2PMLFlags[i] = EncodePATAsPMLFlag(i);
	}
	// Otherwise, reroute all requests to the backwards compatible scheme.
	// TODO: Not all early 32-bit x86 CPUs supports these values.
	else
	{
		PAT2PMLFlags[PAT_UC] = PML_WRTHROUGH | PML_NOCACHE;
		PAT2PMLFlags[PAT_WC] = PML_WRTHROUGH | PML_NOCACHE; // Approx.
		PAT2PMLFlags[2] = 0; // No such flag.
		PAT2PMLFlags[3] = 0; // No such flag.
		PAT2PMLFlags[PAT_WT] = PML_WRTHROUGH;
		PAT2PMLFlags[PAT_WP] = PML_WRTHROUGH; // Approx.
		PAT2PMLFlags[PAT_WB] = 0;
		PAT2PMLFlags[PAT_UCM] = PML_NOCACHE;
	}

	typedef const multiboot_memory_map_t* mmap_t;

	// Loop over every detected memory region.
	for ( mmap_t mmap = (mmap_t) (addr_t) bootinfo->mmap_addr;
	      (addr_t) mmap < bootinfo->mmap_addr + bootinfo->mmap_length;
	      mmap = (mmap_t) ((addr_t) mmap + mmap->size + sizeof(mmap->size)) )
	{
		// Check that we can use this kind of RAM.
		if ( mmap->type != 1 )
			continue;

		// Truncate the memory area if needed.
		uint64_t mmap_addr = mmap->addr;
		uint64_t mmap_len = mmap->len;
#if defined(__i386__)
		if ( 0xFFFFFFFFULL < mmap_addr )
			continue;
		if ( 0xFFFFFFFFULL < mmap_addr + mmap_len )
			mmap_len = 0x100000000ULL - mmap_addr;
#endif

		// Properly page align the entry if needed.
		// TODO: Is the bootloader required to page align this? This could be
		//       raw BIOS data that might not be page aligned? But that would
		//       be a silly computer.
		addr_t base_unaligned = (addr_t) mmap_addr;
		addr_t base = Page::AlignUp(base_unaligned);
		if ( mmap_len < base - base_unaligned )
			continue;
		size_t length_unaligned = mmap_len - (base - base_unaligned);
		size_t length = Page::AlignDown(length_unaligned);
		if ( !length )
			continue;

		// Count the amount of usable RAM.
		Page::totalmem += length;

		// Give all the physical memory to the physical memory allocator
		// but make sure not to give it things we already use.
		addr_t processed = base;
		while ( processed < base + length )
		{
			size_t distance = base + length - processed;
			if ( !CheckUsedRanges(bootinfo, processed, &distance) )
				Page::InitPushRegion(processed, distance);
			processed += distance;
		}
	}

	// Prepare the non-forkable kernel PMLs such that forking the kernel address
	// space will always keep the kernel mapped.
	for ( size_t i = ENTRIES / 2; i < ENTRIES; i++ )
	{
		PML* const pml = PMLS[TOPPMLLEVEL];
		if ( pml->entry[i] & PML_PRESENT )
			continue;

		addr_t page = Page::Get(PAGE_USAGE_PAGING_OVERHEAD);
		if ( !page )
			Panic("Out of memory allocating boot PMLs.");

		pml->entry[i] = page | PML_WRITABLE | PML_PRESENT;

		// Invalidate the new PML and reset it to zeroes.
		addr_t pmladdr = (addr_t) (PMLS[TOPPMLLEVEL-1] + i);
		InvalidatePage(pmladdr);
		memset((void*) pmladdr, 0, sizeof(PML));
	}
}

void Statistics(size_t* amountused, size_t* totalmem)
{
	size_t memfree = (Page::stackused - Page::stackreserved) << 12UL;
	size_t memused = Page::totalmem - memfree;
	if ( amountused )
		*amountused = memused;
	if ( totalmem )
		*totalmem = Page::totalmem;
}

} // namespace Memory
} // namespace Sortix

namespace Sortix {
namespace Page {

void PageUsageRegisterUse(addr_t where, enum page_usage usage)
{
	if ( PAGE_USAGE_NUM_KINDS <= usage )
		return;
	(void) where;
	page_usage_counts[usage]++;
}

void PageUsageRegisterFree(addr_t where, enum page_usage usage)
{
	if ( PAGE_USAGE_NUM_KINDS <= usage )
		return;
	(void) where;
	assert(page_usage_counts[usage] != 0);
	page_usage_counts[usage]--;
}

void ExtendStack()
{
	// This call will always succeed, if it didn't, then the stack
	// wouldn't be full, and thus this function won't be called.
	addr_t page = GetUnlocked(PAGE_USAGE_PHYSICAL);

	// This call will also succeed, since there are plenty of physical
	// pages available and it might need some.
	addr_t virt = (addr_t) (STACK + stacklength);
	if ( !Memory::Map(page, virt, PROT_KREAD | PROT_KWRITE) )
		Panic("Unable to extend page stack, which should have worked");

	// TODO: This may not be needed during the boot process!
	//Memory::InvalidatePage((addr_t) (STACK + stacklength));

	stacklength += 4096UL / sizeof(addr_t);
}

void InitPushRegion(addr_t position, size_t length)
{
	// Align our entries on page boundaries.
	addr_t newposition = Page::AlignUp(position);
	length = Page::AlignDown((position + length) - newposition);
	position = newposition;

	while ( length )
	{
		if ( unlikely(stackused == stacklength) )
		{
			if ( stackused == MAXSTACKLENGTH )
			{
				pagesnotonstack += length / 4096UL;
				return;
			}

			ExtendStack();
		}

		addr_t* stackentry = &(STACK[stackused++]);
		*stackentry = position;

		length -= 4096UL;
		position += 4096UL;
	}
}

bool ReserveUnlocked(size_t* counter, size_t least, size_t ideal)
{
	assert(least < ideal);
	size_t available = stackused - stackreserved;
	if ( least < available )
		return errno = ENOMEM, false;
	if ( available < ideal )
		ideal = available;
	stackreserved += ideal;
	*counter += ideal;
	return true;
}

bool Reserve(size_t* counter, size_t least, size_t ideal)
{
	ScopedLock lock(&pagelock);
	return ReserveUnlocked(counter, least, ideal);
}

bool ReserveUnlocked(size_t* counter, size_t amount)
{
	return ReserveUnlocked(counter, amount, amount);
}

bool Reserve(size_t* counter, size_t amount)
{
	ScopedLock lock(&pagelock);
	return ReserveUnlocked(counter, amount);
}

addr_t GetReservedUnlocked(size_t* counter, enum page_usage usage)
{
	if ( !*counter )
		return 0;
	assert(stackused); // After all, we did _reserve_ the memory.
	addr_t result = STACK[--stackused];
	assert(result == AlignDown(result));
	stackreserved--;
	(*counter)--;
	PageUsageRegisterUse(result, usage);
	return result;
}

addr_t GetReserved(size_t* counter, enum page_usage usage)
{
	ScopedLock lock(&pagelock);
	return GetReservedUnlocked(counter, usage);
}

addr_t GetUnlocked(enum page_usage usage)
{
	assert(stackreserved <= stackused);
	if ( unlikely(stackreserved == stackused) )
		return errno = ENOMEM, 0;
	addr_t result = STACK[--stackused];
	assert(result == AlignDown(result));
	PageUsageRegisterUse(result, usage);
	return result;
}

addr_t Get(enum page_usage usage)
{
	ScopedLock lock(&pagelock);
	return GetUnlocked(usage);
}

// TODO: This competes with the normal allocation for precious 32-bit pages, we
//       should use different pools for this, and preferably preallocate some
//       32-bit pages exclusively for driver usage. Also, get proper hardware
//       without these issues.
addr_t Get32BitUnlocked(enum page_usage usage)
{
	assert(stackreserved <= stackused);
	if ( unlikely(stackreserved == stackused) )
		return errno = ENOMEM, 0;
	for ( size_t ii = stackused; 0 < ii; ii-- )
	{
		size_t i = ii - 1;
		addr_t result = STACK[i];
		assert(result == AlignDown(result));
		if ( 4 < sizeof(void*) && UINT32_MAX < result )
			continue;
		if ( i + 1 != stackused )
		{
			STACK[i] = STACK[stackused - 1];
			STACK[stackused - 1] = result;
		}
		stackused--;
		PageUsageRegisterUse(result, usage);
		return result;
	}
	return errno = ENOMEM, 0;
}

addr_t Get32Bit(enum page_usage usage)
{
	ScopedLock lock(&pagelock);
	return Get32BitUnlocked(usage);
}

void PutUnlocked(addr_t page, enum page_usage usage)
{
	assert(page == AlignDown(page));
	if ( unlikely(stackused == stacklength) )
	{
		if ( stackused == MAXSTACKLENGTH )
		{
			pagesnotonstack++;
			return;
		}
		ExtendStack();
	}
	STACK[stackused++] = page;
	PageUsageRegisterFree(page, usage);
}

void Put(addr_t page, enum page_usage usage)
{
	ScopedLock lock(&pagelock);
	PutUnlocked(page, usage);
}

void Lock()
{
	kthread_mutex_lock(&pagelock);
}

void Unlock()
{
	kthread_mutex_unlock(&pagelock);
}

} // namespace Page
} // namespace Sortix

namespace Sortix {
namespace Memory {

addr_t ProtectionToPMLFlags(int prot)
{
	addr_t result = PML_NX;
	if ( prot & PROT_EXEC )
	{
		result |= PML_USERSPACE;
		result &= ~PML_NX;
	}
	if ( prot & PROT_READ )
		result |= PML_USERSPACE;
	if ( prot & PROT_WRITE )
		result |= PML_USERSPACE | PML_WRITABLE;
	if ( prot & PROT_KEXEC )
		result &= ~PML_NX;
	if ( prot & PROT_KREAD )
		result |= 0;
	if ( prot & PROT_KWRITE )
		result |= PML_WRITABLE;
	if ( prot & PROT_FORK )
		result |= PML_FORK;
	return result;
}

int PMLFlagsToProtection(addr_t flags)
{
	int prot = PROT_KREAD;
	if ( (flags & PML_USERSPACE) && !(flags & PML_NX) )
		prot |= PROT_EXEC;
	if ( (flags & PML_USERSPACE) )
		prot |= PROT_READ;
	if ( (flags & PML_USERSPACE) && (flags & PML_WRITABLE) )
		prot |= PROT_WRITE;
	if ( !(flags & PML_NX) )
		prot |= PROT_KEXEC;
	if ( flags & PML_WRITABLE )
		prot |= PROT_KWRITE;
	if ( flags & PML_FORK )
		prot |= PROT_FORK;
	return prot;
}

int ProvidedProtection(int prot)
{
	return PMLFlagsToProtection(ProtectionToPMLFlags(prot));
}

bool LookUp(addr_t mapto, addr_t* physical, int* protection)
{
	// Translate the virtual address into PML indexes.
	const size_t MASK = (1<<TRANSBITS)-1;
	size_t pmlchildid[TOPPMLLEVEL + 1];
	for ( size_t i = 1; i <= TOPPMLLEVEL; i++ )
		pmlchildid[i] = mapto >> (12 + (i-1) * TRANSBITS) & MASK;

	int prot = PROT_USER | PROT_KERNEL | PROT_FORK;

	// For each PML level, make sure it exists.
	size_t offset = 0;
	for ( size_t i = TOPPMLLEVEL; i > 1; i-- )
	{
		size_t childid = pmlchildid[i];
		PML* pml = PMLS[i] + offset;

		addr_t entry = pml->entry[childid];
		if ( !(entry & PML_PRESENT) )
			return false;
		addr_t entryflags = entry & ~PML_ADDRESS;
		int entryprot = PMLFlagsToProtection(entryflags);
		prot &= entryprot;

		// Find the index of the next PML in the fractal mapped memory.
		offset = offset * ENTRIES + childid;
	}

	addr_t entry = (PMLS[1] + offset)->entry[pmlchildid[1]];
	if ( !(entry & PML_PRESENT) )
		return false;

	addr_t entryflags = entry & ~PML_ADDRESS;
	int entryprot = PMLFlagsToProtection(entryflags);
	prot &= entryprot;
	addr_t phys = entry & PML_ADDRESS;

	if ( physical )
		*physical = phys;
	if ( protection )
		*protection = prot;

	return true;
}

void InvalidatePage(addr_t /*addr*/)
{
	// TODO: Actually just call the instruction.
	Flush();
}


addr_t GetAddressSpace()
{
	addr_t result;
	asm ( "mov %%cr3, %0" : "=r"(result) );
	return result;
}

addr_t SwitchAddressSpace(addr_t addrspace)
{
	assert(Page::IsAligned(addrspace));

	addr_t previous = GetAddressSpace();
	asm volatile ( "mov %0, %%cr3" : : "r"(addrspace) );
	return previous;
}

void Flush()
{
	addr_t previous;
	asm ( "mov %%cr3, %0" : "=r"(previous) );
	asm volatile ( "mov %0, %%cr3" : : "r"(previous) );
}

bool MapRange(addr_t where, size_t bytes, int protection, enum page_usage usage)
{
	for ( addr_t page = where; page < where + bytes; page += 4096UL )
	{
		addr_t physicalpage = Page::Get(usage);
		if ( physicalpage == 0 )
		{
			while ( where < page )
			{
				page -= 4096UL;
				physicalpage = Unmap(page);
				Page::Put(physicalpage, usage);
			}
			return false;
		}

		Map(physicalpage, page, protection);
	}

	return true;
}

bool UnmapRange(addr_t where, size_t bytes, enum page_usage usage)
{
	for ( addr_t page = where; page < where + bytes; page += 4096UL )
	{
		addr_t physicalpage = Unmap(page);
		Page::Put(physicalpage, usage);
	}
	return true;
}

static bool MapInternal(addr_t physical, addr_t mapto, int prot, addr_t extraflags = 0)
{
	addr_t flags = ProtectionToPMLFlags(prot) | PML_PRESENT;

	// Translate the virtual address into PML indexes.
	const size_t MASK = (1<<TRANSBITS)-1;
	size_t pmlchildid[TOPPMLLEVEL + 1];
	for ( size_t i = 1; i <= TOPPMLLEVEL; i++ )
		pmlchildid[i] = mapto >> (12 + (i-1) * TRANSBITS) & MASK;

	// For each PML level, make sure it exists.
	size_t offset = 0;
	for ( size_t i = TOPPMLLEVEL; i > 1; i-- )
	{
		size_t childid = pmlchildid[i];
		PML* pml = PMLS[i] + offset;

		addr_t& entry = pml->entry[childid];

		// Find the index of the next PML in the fractal mapped memory.
		size_t childoffset = offset * ENTRIES + childid;

		if ( !(entry & PML_PRESENT) )
		{
			// TODO: Possible memory leak when page allocation fails.
			addr_t page = Page::Get(PAGE_USAGE_PAGING_OVERHEAD);

			if ( !page )
				return false;
			addr_t pmlflags = PML_PRESENT | PML_WRITABLE | PML_USERSPACE
			                | PML_FORK;
			entry = page | pmlflags;

			// Invalidate the new PML and reset it to zeroes.
			addr_t pmladdr = (addr_t) (PMLS[i-1] + childoffset);
			InvalidatePage(pmladdr);
			memset((void*) pmladdr, 0, sizeof(PML));
		}

		offset = childoffset;
	}

	// Actually map the physical page to the virtual page.
	const addr_t entry = physical | flags | extraflags;
	(PMLS[1] + offset)->entry[pmlchildid[1]] = entry;
	return true;
}

bool Map(addr_t physical, addr_t mapto, int prot)
{
	return MapInternal(physical, mapto, prot);
}

void PageProtect(addr_t mapto, int protection)
{
	addr_t phys;
	if ( !LookUp(mapto, &phys, NULL) )
		return;
	Map(phys, mapto, protection);
}

void PageProtectAdd(addr_t mapto, int protection)
{
	addr_t phys;
	int prot;
	if ( !LookUp(mapto, &phys, &prot) )
		return;
	prot |= protection;
	Map(phys, mapto, prot);
}

void PageProtectSub(addr_t mapto, int protection)
{
	addr_t phys;
	int prot;
	if ( !LookUp(mapto, &phys, &prot) )
		return;
	prot &= ~protection;
	Map(phys, mapto, prot);
}

addr_t Unmap(addr_t mapto)
{
	// Translate the virtual address into PML indexes.
	const size_t MASK = (1<<TRANSBITS)-1;
	size_t pmlchildid[TOPPMLLEVEL + 1];
	for ( size_t i = 1; i <= TOPPMLLEVEL; i++ )
	{
		pmlchildid[i] = mapto >> (12 + (i-1) * TRANSBITS) & MASK;
	}

	// For each PML level, make sure it exists.
	size_t offset = 0;
	for ( size_t i = TOPPMLLEVEL; i > 1; i-- )
	{
		size_t childid = pmlchildid[i];
		PML* pml = PMLS[i] + offset;

		addr_t& entry = pml->entry[childid];

		if ( !(entry & PML_PRESENT) )
			PanicF("Attempted to unmap virtual page 0x%jX, but the virtual"
			       " page was wasn't mapped. This is a bug in the code "
			       "code calling this function", (uintmax_t) mapto);

		// Find the index of the next PML in the fractal mapped memory.
		offset = offset * ENTRIES + childid;
	}

	addr_t& entry = (PMLS[1] + offset)->entry[pmlchildid[1]];
	addr_t result = entry & PML_ADDRESS;
	entry = 0;

	// TODO: If all the entries in PML[N] are not-present, then who
	// unmaps its entry from PML[N-1]?

	return result;
}

bool MapPAT(addr_t physical, addr_t mapto, int prot, addr_t mtype)
{
	addr_t extraflags = PAT2PMLFlags[mtype];
	return MapInternal(physical, mapto, prot, extraflags);
}

void ForkCleanup(size_t i, size_t level)
{
	PML* destpml = FORKPML + level;
	if ( !i )
		return;
	for ( size_t n = 0; n < i-1; n++ )
	{
		addr_t entry = destpml->entry[i];
		if ( !(entry & PML_FORK ) )
			continue;
		addr_t phys = entry & PML_ADDRESS;
		if ( 1 < level )
		{
			addr_t destaddr = (addr_t) (FORKPML + level-1);
			Map(phys, destaddr, PROT_KREAD | PROT_KWRITE);
			InvalidatePage(destaddr);
			ForkCleanup(ENTRIES+1UL, level-1);
		}
		enum page_usage usage = 1 < level ? PAGE_USAGE_PAGING_OVERHEAD
		                                  : PAGE_USAGE_USER_SPACE;
		Page::Put(phys, usage);
	}
}

// TODO: Copying every frame is endlessly useless in many uses. It'd be
// nice to upgrade this to a copy-on-write algorithm.
bool Fork(size_t level, size_t pmloffset)
{
	PML* destpml = FORKPML + level;
	for ( size_t i = 0; i < ENTRIES; i++ )
	{
		addr_t entry = (PMLS[level] + pmloffset)->entry[i];

		// Link the entry if it isn't supposed to be forked.
		if ( !(entry & PML_PRESENT) || !(entry & PML_FORK ) )
		{
			destpml->entry[i] = entry;
			continue;
		}

		enum page_usage usage = 1 < level ? PAGE_USAGE_PAGING_OVERHEAD
		                                  : PAGE_USAGE_USER_SPACE;
		addr_t phys = Page::Get(usage);
		if ( unlikely(!phys) )
		{
			ForkCleanup(i, level);
			return false;
		}

		addr_t flags = entry & PML_FLAGS;
		destpml->entry[i] = phys | flags;

		// Map the destination page.
		addr_t destaddr = (addr_t) (FORKPML + level-1);
		Map(phys, destaddr, PROT_KREAD | PROT_KWRITE);
		InvalidatePage(destaddr);

		size_t offset = pmloffset * ENTRIES + i;

		if ( 1 < level )
		{
			if ( !Fork(level-1, offset) )
			{
				Page::Put(phys, usage);
				ForkCleanup(i, level);
				return false;
			}
			continue;
		}

		// Determine the source page's address.
		const void* src = (const void*) (offset * 4096UL);

		// Determine the destination page's address.
		void* dest = (void*) (FORKPML + level - 1);

		memcpy(dest, src, 4096UL);
	}

	return true;
}

bool Fork(addr_t dir, size_t level, size_t pmloffset)
{
	PML* destpml = FORKPML + level;

	// This call always succeeds.
	Map(dir, (addr_t) destpml, PROT_KREAD | PROT_KWRITE);
	InvalidatePage((addr_t) destpml);

	return Fork(level, pmloffset);
}

// Create an exact copy of the current address space.
addr_t Fork()
{
	addr_t dir = Page::Get(PAGE_USAGE_PAGING_OVERHEAD);
	if ( dir == 0 )
		return 0;
	if ( !Fork(dir, TOPPMLLEVEL, 0) )
	{
		Page::Put(dir, PAGE_USAGE_PAGING_OVERHEAD);
		return 0;
	}

	// Now, the new top pml needs to have its fractal memory fixed.
	const addr_t flags = PML_PRESENT | PML_WRITABLE;
	addr_t mapto;
	addr_t childaddr;

	(FORKPML + TOPPMLLEVEL)->entry[ENTRIES-1] = dir | flags;
	childaddr = (FORKPML + TOPPMLLEVEL)->entry[ENTRIES-2] & PML_ADDRESS;

	for ( size_t i = TOPPMLLEVEL-1; i > 0; i-- )
	{
		mapto = (addr_t) (FORKPML + i);
		Map(childaddr, mapto, PROT_KREAD | PROT_KWRITE);
		InvalidatePage(mapto);
		(FORKPML + i)->entry[ENTRIES-1] = dir | flags;
		childaddr = (FORKPML + i)->entry[ENTRIES-2] & PML_ADDRESS;
	}
	return dir;
}

} // namespace Memory
} // namespace Sortix

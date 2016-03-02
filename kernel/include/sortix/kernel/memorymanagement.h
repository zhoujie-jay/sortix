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
 * sortix/kernel/memorymanagement.h
 * Functions that allow modification of virtual memory.
 */

#ifndef INCLUDE_SORTIX_KERNEL_MEMORYMANAGEMENT_H
#define INCLUDE_SORTIX_KERNEL_MEMORYMANAGEMENT_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/decl.h>

typedef struct multiboot_info multiboot_info_t;

namespace Sortix {

class Process;

enum page_usage
{
	PAGE_USAGE_OTHER,
	PAGE_USAGE_PHYSICAL,
	PAGE_USAGE_PAGING_OVERHEAD,
	PAGE_USAGE_KERNEL_HEAP,
	PAGE_USAGE_FILESYSTEM_CACHE,
	PAGE_USAGE_USER_SPACE,
	PAGE_USAGE_EXECUTE,
	PAGE_USAGE_DRIVER,
	PAGE_USAGE_NUM_KINDS,
	PAGE_USAGE_WASNT_ALLOCATED,
};

} // namespace Sortix

namespace Sortix {
namespace Page {

bool Reserve(size_t* counter, size_t amount);
bool ReserveUnlocked(size_t* counter, size_t amount);
bool Reserve(size_t* counter, size_t least, size_t ideal);
bool ReserveUnlocked(size_t* counter, size_t least, size_t ideal);
addr_t GetReserved(size_t* counter, enum page_usage usage);
addr_t GetReservedUnlocked(size_t* counter, enum page_usage usage);
addr_t Get(enum page_usage usage);
addr_t GetUnlocked(enum page_usage usage);
addr_t Get32Bit(enum page_usage usage);
addr_t Get32BitUnlocked(enum page_usage usage);
void Put(addr_t page, enum page_usage usage);
void PutUnlocked(addr_t page, enum page_usage usage);
void Lock();
void Unlock();

inline size_t Size() { return 4096UL; }

// Rounds a memory address down to nearest page.
inline addr_t AlignDown(addr_t page) { return page & ~(0xFFFUL); }

// Rounds a memory address up to nearest page.
inline addr_t AlignUp(addr_t page) { return AlignDown(page + 0xFFFUL); }

// Tests whether an address is page aligned.
inline bool IsAligned(addr_t page) { return AlignDown(page) == page; }

} // namespace Page
} // namespace Sortix

namespace Sortix {
namespace Memory {

void Init(multiboot_info_t* bootinfo);
void InvalidatePage(addr_t addr);
void Flush();
addr_t Fork();
addr_t GetAddressSpace();
addr_t SwitchAddressSpace(addr_t addrspace);
void DestroyAddressSpace(addr_t fallback);
bool Map(addr_t physical, addr_t mapto, int prot);
addr_t Unmap(addr_t mapto);
addr_t Physical(addr_t mapto);
int PageProtection(addr_t mapto);
bool LookUp(addr_t mapto, addr_t* physical, int* prot);
int ProvidedProtection(int prot);
void PageProtect(addr_t mapto, int protection);
void PageProtectAdd(addr_t mapto, int protection);
void PageProtectSub(addr_t mapto, int protection);
bool MapRange(addr_t where, size_t bytes, int protection, enum page_usage usage);
bool UnmapRange(addr_t where, size_t bytes, enum page_usage usage);
void Statistics(size_t* amountused, size_t* totalmem);
addr_t GetKernelStack();
size_t GetKernelStackSize();
void GetKernelVirtualArea(addr_t* from, size_t* size);
void GetUserVirtualArea(uintptr_t* from, size_t* size);
void UnmapMemory(Process* process, uintptr_t addr, size_t size);
bool ProtectMemory(Process* process, uintptr_t addr, size_t size, int prot);
bool MapMemory(Process* process, uintptr_t addr, size_t size, int prot);

} // namespace Memory
} // namespace Sortix

#endif

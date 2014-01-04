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

    sortix/kernel/memorymanagement.h
    Functions that allow modification of virtual memory.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_MEMORYMANAGEMENT_H
#define INCLUDE_SORTIX_KERNEL_MEMORYMANAGEMENT_H

typedef struct multiboot_info multiboot_info_t;

namespace Sortix {

class Process;

} // namespace Sortix

namespace Sortix {
namespace Page {

bool Reserve(size_t* counter, size_t amount);
bool ReserveUnlocked(size_t* counter, size_t amount);
bool Reserve(size_t* counter, size_t least, size_t ideal);
bool ReserveUnlocked(size_t* counter, size_t least, size_t ideal);
addr_t GetReserved(size_t* counter);
addr_t GetReservedUnlocked(size_t* counter);
addr_t Get();
addr_t GetUnlocked();
void Put(addr_t page);
void PutUnlocked(addr_t page);
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
void DestroyAddressSpace(addr_t fallback = 0,
                         void (*func)(addr_t, void*) = NULL,
                         void* user = NULL);
bool Map(addr_t physical, addr_t mapto, int prot);
addr_t Unmap(addr_t mapto);
addr_t Physical(addr_t mapto);
int PageProtection(addr_t mapto);
bool LookUp(addr_t mapto, addr_t* physical, int* prot);
int ProvidedProtection(int prot);
void PageProtect(addr_t mapto, int protection);
void PageProtectAdd(addr_t mapto, int protection);
void PageProtectSub(addr_t mapto, int protection);
bool MapRange(addr_t where, size_t bytes, int protection);
bool UnmapRange(addr_t where, size_t bytes);
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

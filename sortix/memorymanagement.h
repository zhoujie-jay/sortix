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

	memorymanagement.h
	Handles memory for the x86 architecture.

******************************************************************************/

#ifndef SORTIX_MEMORYMANAGEMENT_H
#define SORTIX_MEMORYMANAGEMENT_H

// Forward declarations.
typedef struct multiboot_info multiboot_info_t;

namespace Sortix
{
	// This is the physical page allocator API. It splits the physical memory
	// of the local machine into chunks known as pages. Each page is usually
	// 4096 bytes (depends on your CPU). Pages are page-aligned, meaning they
	// all are located on a multiple of the page size (such as 4096 byte).
	//
	// A long list of memory addresses is used to store and retrieve page
	// addresses from. To allocate a physical page of memory, simply call the
	// Page::Get() function. When you are done using it, you can free it using
	// the Page::Put() function, which makes the page available for other uses.
	//
	// To use a physical page, the CPU must be in physical mode. Since it is
	// undesirable to be in physical mode, using the physical page requires
	// using the virtual memory API (see below).
	//
	// This API completely bypasses the memory swapping system.
	//
	// If you just want memory for use by the kernel, allocate it using 'new'.
	namespace Page
	{
		// Initializes the paging system. Accepts a multiboot structure
		// containing the layout of the physical memory in this machine.
		void Init(multiboot_info_t* bootinfo);

		// Allocates a physical page and returns its physical address, or
		// returns 0 if no page currently is available in the system.
		addr_t Get();

		// Deallocates a physical page allocated using Get(), which lets the
		// the system reuse the page for other purposes. Page must have been
		// allocated using Get().
		void Put(addr_t page);

		// Inserts a physical page into the paging system. This page must not
		// have been allocated using Get() and must have been allocated safely
		// through other means (such as information provided by the bootloader).
		void Insert(addr_t page);

		// Rounds a memory address down to nearest page.
		inline addr_t AlignDown(addr_t page) { return page & ~(0xFFFUL); }

		// Rounds a memory address up to nearest page.
		inline addr_t AlignUp(addr_t page) { return AlignDown(page + 0xFFFUL); }

		// Retrieves statistics about the current page usage in the system, how
		// big pages are, now many are free, and how many are used. Each
		// parameter must be a legal pointer to the locations wherein the stats
		// will be stored. This function always succeeds. 
		void GetStats(size_t* pagesize, size_t* numfree, size_t* numused);
	}

	// This the virtual memory API. Virtual Memory is a clever way to make the
	// RAM just as you want it. In effect, it it makes the RAM completely empty
	// (there is no RAM). You can then add memory where you desire. You can take
	// a physical page and put it in several places, and you can even add
	// permissions to it (read-only, read-write, kernel-only). Naturally, the
	// amount of physical pages is a limit (out of memory).
	//
	// There can exist several virtual address spaces, and it is possible for
	// them to share physical pages. Each process in the system has its own
	// virtual address space, but the kernel is always mapped in each address
	// space. While the kernel can access all of the virtual address space,
	// user-space programs can only access what they are allowed, cannot access
	// the kernel's memory and cannot access each other's address spaces. This
	// prevents programs from tampering with each other and from bringing the
	// whole system down.
	//
	// Sortix has several conventions for the layout of the virtual address
	// space. The kernel uses the top of the address space, and user-space is
	// generally allowed to use the bottom and the middle for stuff such as
	// program code, variables, the stack, the heap, and other stuff.
	//
	// To access physical memory, you must allocate a physical page of memory
	// and map it to a virtual address. You can then modify the memory through
	// a pointer to that address.
	//
	// You should select the correct set of functions when writing new code.
	// Using the functions incorrectly, using the wrong functions, or mixing
	// incompatible functions can lead to gruesome bugs.
	//
	// For conveniece, the functions have been grouped. Combining (un)mapping
	// functions from groups is bad style and is possibly buggy. Assertions may
	// be present to detect bad combination.
	//
	// If you modify kernel virtual pages, then the effects will be share across
	// all virtual address spaces.
	//
	// If you modify user-space virtual pages, then the effects will be limited
	// to the current process and its personal virtual address space.
	//
	// If you just want memory for use by the kernel, allocate it using 'new'.
	namespace VirtualMemory
	{
		// Initializes the virtual memory system. This bootstraps the kernel
		// paging system (if needed) such that the initial kernel's virtual
		// address space is  created and the current process's page structures
		// are mapped to conventional locations.
		// This system depends on the physical page allocator being initialized.
		void Init();

		// Creates a new, fresh address space only containing the kernel memory
		// regions, and a fresh user-space frontier of nothingness. The current
		// address space remains active and is not modified (besides some kernel
		// pages used for this purpose). Returns the physical address of the new
		// top level paging structure, which is an opaque identifier as it is
		// not identity mapped. Returns 0 if insufficient memory was available.
		addr_t CreateAddressSpace();

		// Switches the current address space to the virtual address space
		// 'addrspace', which must be the result of CreateAddressSpace() or
		// another function that creates or copies address spaces.
		addr_t SwitchAddressSpace(addr_t addrspace);

		// =====================================================================
		// Function Group 1.
		// Mapping a single kernel page. 
		//
		// These functions allow you to map a single physical page for use by
		// the kernel.
		//
		// Usage: 
		//
		// addr_t physicalpage = Page::Get();
		// if ( physicalpage == 0 ) { /* handle error */ }
		//
		// const addr_t mapto = 0xF00;
		// VirtualMemory::MapKernel(mapto, physicalpage);
		//
		// /* access the memory */
		// char* mem = (char*) mapto;
		// mem[0] = 'K';
		// 
		// ...
		//
		// addr_t physpage = VirtualMemory::UnmapKernel(mapto);
		//
		// /* when physpage is no longer referred, free it */
		// Page::Put(physpage);

		// Maps the physical page 'physical' to the virtual address 'where',
		// with read and write flags set, but only accessible to the kernel.
		// 'where' must be a virtual address in the range available to the
		// kernel, and must not currently be used. Given legal input, this
		// function will always succeed - illegal input is a gruesome bug.
		// 'where' must be page aligned. The effect of this function will be
		// shared in all addresses spaces - it is a global change.
		// You are allowed to map the same physical page multiple times, even
		// with Group 2. functions, just make sure to call the proper Unmap
		// functions for each virtual address you map it to, and don't free the
		// physical page until it is no longer referred to.
		// The virtual page 'where' will point to 'physical' instantly after
		// this function returns.
		void MapKernel(addr_t where, addr_t physical);

		// This function is equal to unmapping 'where', then mapping 'where' to
		// 'newphysical' and returns previous physical page 'where' pointed to.
		// The same requirements for MapKernel and UnmapKernel applies.
		addr_t RemapKernel(addr_t where, addr_t newphysical);

		// Unmaps the virtual address 'where' such that it no longer points to
		// a valid physical address. Accessing the page at the virtual address
		// 'where' will result in an access violation (panicing/crashing the
		// kernel). 'where' must be a virtual address already mapped in the
		// kernel virtual memory ranges. 'where' must be page aligned.
		// 'where' must already be a legal kernel virtual page.
		// Returns the address of the physical memory page 'where' points to
		// before being cleared. Before returning the physical page to the page
		// allocator, make sure that it is not used by other virtual pages.
		addr_t UnmapKernel(addr_t where);

		// =====================================================================
		// Function Group 2.
		// Mapping a single user-space page. 
		//
		// These functions allow you to map a single physical page for use by
		// user-space programs.
		//
		// Usage: 
		//
		// addr_t physicalpage = Page::Get();
		// if ( physicalpage == 0 ) { /* handle error */ }
		//
		// const addr_t mapto = 0xF00;
		// if ( !VirtualMemory::MapUser(mapto, physicalpage) )
		// {
		//      Page::Put(physicalpage);
		//      /* handle error */
		// }
		//
		// /* let user-space use memory safely */
		//
		// addr_t physpage = VirtualMemory::UnmapUser(mapto);
		//
		// /* when physpage is no longer referred, free it */
		// Page::Put(physpage);

		// Maps the physical page 'physical' to the virtual address 'where',
		// with read and write flags set, accessible to both kernel and user-
		// space. 'where' must be a virtual address not in the kernel ranges,
		// that is, available to userspace. 'where' must be page aligned and
		// not currently used. Returns false if insufficient memory is available
		// and returns with the address space left unaltered. Illegal input is
		// also a gruesome bug. This function only changes the address space of
		// the current process.
		// You are allowed to map the same physical page multiple times, even
		// with Group 1. functions, just make sure to call the proper Unmap
		// functions for each virtual address you map it to, and don't free the
		// physical page until it is no longer referred to.
		// The virtual page 'where' will point to 'physical' instantly after
		// this function returns.
		bool MapUser(addr_t where, addr_t physical);

		// This function is equal to unmapping 'where', then mapping 'where' to
		// 'newphysical' and returns previous physical page 'where' pointed to.
		// The same requirements for MapUser and UnmapUser applies.
		addr_t RemapUser(addr_t where, addr_t newphysical);

		// Unmaps the virtual address 'where' such that it no longer points to
		// a valid physical address. Accessing the page at the virtual address
		// 'where' will result in an access violation (panicing/crashing the
		// program/kernel). 'where' must be a virtual address already mapped
		// outside  the kernel virtual memory ranges. 'where' must be page
		//aligned. 'where' must already be a legal user-space virtual page.
		// Returns the address of the physical memory page 'where' points to
		// before being cleared. Before returning the physical page to the page
		// allocator, make sure that it is not used by other virtual pages.
		addr_t UnmapUser(addr_t where);

		// =====================================================================
		// Function Group 3.
		// Mapping a range of kernel pages.
		//
		// These functions allow you to specify a range of virtual pages that
		// shall be usable by the kernel. Memory will be allocated accordingly.
		//
		// Usage: 
		//
		// const addr_t mapto = 0x4000UL;
		// size_t numpages = 8;
		// size_t bytes = numpages * 4096UL;
		// if ( !VirtualMemory::MapRangeKernel(mapto, bytes) )
		// {
		//     /* handle error */
		// }
		//
		// /* use memory here */
		// ...
		//
		// VirtualMemory::UnmapRangeKernel(mapto, bytes);
		
		// Allocates the needed pages and maps them to 'where'. Returns false if
		// not enough memory is available. 'where' must be page aligned. 'bytes'
		// need not be page aligned and is rounded up to nearest page. The
		// region of 'where' and onwards must not be currently mapped.
		// The memory will only be readable and writable by the kernel.
		// The memory will be available the instant the function returns.
		// The whole region must be within the kernel virtual page ranges.
		bool MapRangeKernel(addr_t where, size_t bytes);

		// Deallocates the selected pages, and unmaps the selected region.
		// 'where' must be paged aligned. 'bytes' need not be page aligned and
		// is rounded up to nearest page. Each page in the region must have been
		// allocated by MapRangeKernel. You need not free a whole region at once
		// and you may even combine pages from adjacent regions.
		void UnmapRangeKernel(addr_t where, size_t bytes);

		// =====================================================================
		// Function Group 4.
		// Mapping a range of user-space pages.
		//
		// These functions allow you to specify a range of virtual pages that
		// shall be usable by the user-space. Memory will be allocated
		// accordingly.
		//
		// Usage: 
		//
		// const addr_t mapto = 0x4000UL;
		// size_t numpages = 8;
		// size_t bytes = numpages * 4096UL;
		// if ( !VirtualMemory::MapRangeUser(mapto, bytes) )
		// {
		//     /* handle error */
		// }
		//
		// /* let user-space use memory here */
		// ...
		//
		// VirtualMemory::UnmapRangeUser(mapto, bytes);
		
		// Allocates the needed pages and maps them to 'where'. Returns false if
		// not enough memory is available. 'where' must be page aligned. 'bytes'
		// need not be page aligned and is rounded up to nearest page. The
		// region of 'where' and onwards must not be currently mapped.
		// The memory will be readable and writable by user-space.
		// The memory will be available the instant the function returns.
		// The whole region must be outside the kernel virtual page ranges.
		bool MapRangeUser(addr_t where, size_t bytes);

		// Deallocates the selected pages, and unmaps the selected region.
		// 'where' must be paged aligned. 'bytes' need not be page aligned and
		// is rounded up to nearest page. Each page in the region must have been
		// allocated by MapRangeUser. You need not free a whole region at once
		// and you may even combine pages from adjacent regions.
		void UnmapRangeUser(addr_t where, size_t bytes);

#ifdef PLATFORM_X86
		// Define where the kernel heap is located, used by the heap code.
		const addr_t heapLower = 0x80000000UL;
		const addr_t heapUpper = 0xFF800000UL;

		// Physical pages may be safely temporarily mapped to this address and a
		// good dozen of pages onwards. Beware that this is only meant to be
		// a temporary place to put memory.
		const addr_t tempaddr = 0xFF800000UL;
#endif

	}
}

#endif


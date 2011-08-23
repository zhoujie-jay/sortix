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
#include "log.h"
#include "panic.h"
#include "multiboot.h"
#include "memorymanagement.h"

using namespace Maxsi;

namespace Sortix
{
	const addr_t KernelStart = 0x000000UL;
	const size_t KernelLength = 0x200000UL;
	const size_t KernelLeadingPages = KernelLength / 0x1000UL;

	namespace Page
	{
		struct UnallocPage // Must like this due to assembly.
		{
			size_t Magic;
			void* Next;
			size_t ContinuousPages;
		};

		// Refers to private assembly functions.
		addr_t GetPrivate();
		void PutPrivate(addr_t page);
		void Fragmentize();

		UnallocPage* volatile UnallocatedPage; // Must have this name and namespace due to assembly.
		size_t pagesTotal;
		size_t pagesUsed;
		size_t pagesFree;

		const size_t UnallocPageMagic = 0xABBAACDC; // Must this value due to assembly

		// Creates the Great Linked List of All Linked Lists!
		void Init(multiboot_info_t* BootInfo)
		{
			UnallocatedPage = NULL;
			pagesTotal = 0;

			if ( !( BootInfo->flags & MULTIBOOT_INFO_MEM_MAP ) )
			{
				Panic("memorymanagement.cpp: The memory map flag was't set in the multiboot structure. Are your bootloader multiboot compliant?");
			}

			for	(
					multiboot_memory_map_t* MMap = (multiboot_memory_map_t*) BootInfo->mmap_addr;
					(uintptr_t) MMap < BootInfo->mmap_addr + BootInfo->mmap_length;
					MMap = (multiboot_memory_map_t *) ((uintptr_t) MMap + MMap->size + sizeof(MMap->size))
				)
			{

				// Check that we can use this kind of RAM.
				if ( MMap->type != 1 ) { continue; }

				//Log::PrintF("RAM at 0x%64x\t of length 0x%64zx\n", MMap->addr, MMap->len);

				// The kernels code may split this memory area into multiple pieces.
				struct { uintptr_t Base; size_t Length; } Entries[2]; nat Num = 1;

				// Attempt to crop the entry so that we only map what we can address.
				Entries[0].Base = (uintptr_t) MMap->addr;
				Entries[0].Length = MMap->len;
	#ifdef PLATFORM_X86
				// Figure out if the memory area is addressable (are our pointers big enough?)
				if ( 0xFFFFFFFF < MMap->addr ) { continue; }
				if ( 0xFFFFFFFF < MMap->addr + MMap->len ) { Entries[0].Length = 0xFFFFFFFF - MMap->addr;  }
	#endif

				// Detect if this memory is completely covered by the kernel.
				if ( KernelStart <= Entries[0].Base && Entries[0].Base + Entries[0].Length <= KernelStart + KernelLength ) { continue; }

				// Detect if this memory is partially covered by the kernel (from somewhere in the memory to somewhere else in the memory)
				else if ( Entries[0].Base <= KernelStart && KernelStart + KernelLength <= Entries[0].Base + Entries[0].Length )
				{
					Entries[1].Base = KernelStart + KernelLength;
					Entries[1].Length = (Entries[0].Base + Entries[0].Length) - Entries[1].Base;
					Entries[0].Length = KernelStart - Entries[0].Base;
					Num = 2;
				}

				// Detect if this memory is partially covered by the kernel (from the left to somewhere in the memory)
				else if ( Entries[0].Base <= KernelStart + KernelLength && KernelStart + KernelLength <= Entries[0].Base + Entries[0].Length )
				{
					Entries[0].Length = (Entries[0].Base + Entries[0].Length) - (KernelStart + KernelLength);
					Entries[0].Base = KernelStart + KernelLength;
				}

				// Detect if this memory is partially covered by the kernel (from somewhere in the memory to the right)
				else if ( Entries[0].Base <= KernelStart && KernelStart <= Entries[0].Base + Entries[0].Length )
				{
					Entries[0].Length = KernelStart - Entries[0].Base;
				}

				for ( nat I = 0; I < Num; I++ )	
				{

					// Align our entries on page boundaries.
					uintptr_t NewBase = (Entries[I].Base + 0xFFF) / 0x1000 * 0x1000;
					Entries[I].Length = (Entries[I].Base + Entries[I].Length ) - NewBase;
					Entries[I].Length /= 0x1000;
					Entries[I].Base = NewBase;

					if ( Entries[I].Length == 0 ) { continue; }

					#ifdef PLATFORM_X64
					Log::Print("Halt: CPU X64 cannot continue as the virtual memory isn't disabled (kernel bug) and the code is about to access non-mapped memory.\n");
					while(true);
					#endif

					UnallocPage* Page = (UnallocPage*) Entries[I].Base;
					Page->Magic = UnallocPageMagic;
					Page->Next = UnallocatedPage;
					Page->ContinuousPages = Entries[I].Length - 1;

					pagesTotal += Entries[I].Length;

					UnallocatedPage = Page;	
				}
			}

			if ( pagesTotal == 0 ) { Panic("memorymanagement.cpp: no RAM were available for paging"); }

			// Alright, time to make our linked list into a lot of small entries.
			// This speeds up the system when it's fully up and running. It only
			// takes a few miliseconds to run this operation on my laptop.
			Fragmentize();

			pagesFree = pagesTotal;
			pagesUsed = 0;

			ASSERT(pagesFree + pagesUsed == pagesTotal);

#ifndef PLATFORM_SERIAL
			//Log::PrintF("%zu pages are available for paging (%zu MiB RAM)\n", PagesTotal, PagesTotal >> 8 /* * 0x1000 / 1024 / 1024*/);
#endif
		}

		addr_t Get()
		{
			addr_t result = GetPrivate();

			if ( result != 0 )
			{
				ASSERT(pagesFree > 0);
				pagesUsed++;
				pagesFree--;
			}
			else
			{
				ASSERT(pagesFree == 0);
			}

			ASSERT(pagesFree + pagesUsed == pagesTotal);
			return result;
		}

		void Put(addr_t page)
		{
			pagesFree++;
			pagesUsed--;
			
			PutPrivate(page);
		}

		void Insert(addr_t page)
		{
			pagesFree++;
			pagesTotal++;
	
			PutPrivate(page);
		}

		void GetStats(size_t* pagesize, size_t* numfree, size_t* numused)
		{
			*pagesize = 4096UL;
			*numfree = pagesFree;
			*numused = pagesUsed;
		}
	}

	namespace VirtualMemory
	{
		const size_t TABLE_PRESENT		= (1<<0);
		const size_t TABLE_WRITABLE		= (1<<1);
		const size_t TABLE_USER_SPACE	= (1<<2);
		const size_t TABLE_RESERVED1	= (1<<3); // Used internally by the CPU.
		const size_t TABLE_RESERVED2	= (1<<4); // Used internally by the CPU.
		const size_t TABLE_ACCESSED		= (1<<5);
		const size_t TABLE_DIRTY		= (1<<6);
		const size_t TABLE_RESERVED3	= (1<<7); // Used internally by the CPU.
		const size_t TABLE_RESERVED4	= (1<<8); // Used internally by the CPU.
		const size_t TABLE_AVAILABLE1	= (1<<9);
		const size_t TABLE_AVAILABLE2	= (1<<10);
		const size_t TABLE_AVAILABLE3	= (1<<11);
		const size_t TABLE_FLAGS		= (0xFFFUL); // Bits used for the flags.
		const size_t TABLE_ADDRESS		= (~0xFFFUL); // Bits used for the address.

		const size_t DIR_PRESENT		= (1<<0);
		const size_t DIR_WRITABLE		= (1<<1);
		const size_t DIR_USER_SPACE		= (1<<2);
		const size_t DIR_WRITE_THROUGH	= (1<<3);
		const size_t DIR_DISABLE_CACHE	= (1<<4);
		const size_t DIR_ACCESSED		= (1<<5);
		const size_t DIR_RESERVED1		= (1<<6);
		const size_t DIR_4MIB_PAGES		= (1<<7);
		const size_t DIR_RESERVED2		= (1<<8);
		const size_t DIR_AVAILABLE1		= (1<<9);
		const size_t DIR_AVAILABLE2		= (1<<10);
		const size_t DIR_AVAILABLE3		= (1<<11);
		const size_t DIR_FLAGS			= (0xFFFUL); // Bits used for the flags.
		const size_t DIR_ADDRESS		= (~0xFFFUL); // Bits used for the address.

		const size_t ENTRIES = 4096 / sizeof(addr_t);

		struct Table
		{
			addr_t page[ENTRIES];
		};

		struct Dir
		{
			addr_t table[ENTRIES];
		};

	#ifdef PLATFORM_X86
		// These structures are always virtually mapped to these addresses.
		Table*	const		makingTable		=	(Table*)	0xFFBFC000UL;
		Dir*	const		makingDir		=	(Dir*)		0xFFBFD000UL;
		Dir*	const		kernelDir		=	(Dir*)		0xFFBFE000UL;
		Dir*	const		currentDir		=	(Dir*)		0xFFBFF000UL;
		Table*	const		currentTables	=	(Table*)	0xFFC00000UL;
	#endif

	#ifdef PLATFORM_X64
		// TODO: These are dummy values!
		const	Dir*		currentDir		=	(Dir*)		0xFACEB00C;
		const	Table*		currentTables	=	(Table*)	0xFACEB00C;
	#endif

		addr_t		currentDirPhysical;

	#ifdef PLATFORM_X86
		Table* BootstrapCreateTable(Dir* dir, addr_t where);
		void BootstrapMap(Dir* dir, addr_t where, addr_t physical);
		void BootstrapMapStructures(Dir* dir);
		addr_t SwitchDirectory(addr_t dir);
		addr_t CreateDirectory();
	#endif

		void Init()
		{
		#ifdef PLATFORM_X86	

			// Initialize variables.
			currentDirPhysical = 0;

			// Allocate a page we can use for our kernel page directory.
			Dir* dirphys = (Dir*) Page::Get();
			if ( dirphys == NULL ) { Panic("memorymanagement.cpp: Could not allocate page dir"); }

			Memory::Set(dirphys, 0, sizeof(Dir));

			// Identity map the kernel.
			for ( addr_t ptr = KernelStart; ptr < KernelStart + KernelLength; ptr += 0x1000UL )
			{
				BootstrapMap(dirphys, ptr, ptr);
			}

			// Create every table used in the kernel half. We do it now such that
			// any copies of the kernel dir never gets out of date.
			for ( addr_t ptr = 0x80000000UL; ptr != 0UL; ptr += ENTRIES * 0x1000UL )
			{
				BootstrapCreateTable(dirphys, ptr);
			}

			// Map the paging structures themselves.
			BootstrapMapStructures(dirphys);

			// We have now created a minimal virtual environment where the kernel
			// is mapped, the paging structures are ready, and the paging
			// structures are mapped. We are now ready to enable pages.

			// Switch the current dir - this enables paging.
			SwitchDirectory((addr_t) dirphys);

			// Hello, virtual world!			

		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

	#ifdef PLATFORM_X86	
		inline addr_t GetTableId(addr_t where) { return where / (4096UL * ENTRIES); }
		inline addr_t GetPageId(addr_t where) { return ( where / 4096UL ) % ENTRIES; }

		Table* BootstrapCreateTable(Dir* dir, addr_t where)
		{
			size_t tableid = GetTableId(where);
			addr_t tabledesc = dir->table[tableid];

			if ( tabledesc != 0 )
			{
				return (Table*) (tabledesc & TABLE_ADDRESS);
			}
			else
			{
				addr_t tablepage = Page::Get();
				if ( tablepage == 0 )
				{
					PanicF("memorymanagement.cpp: Could not allocate bootstrap page table for 0x%p", where);
				}
				Memory::Set((void*) tablepage, 0, sizeof(Table));
				tabledesc = tablepage | TABLE_PRESENT | TABLE_WRITABLE;
				dir->table[tableid] = tabledesc;
				ASSERT((Table*) tablepage == BootstrapCreateTable(dir, where));
				return (Table*) tablepage;
			}
		}

		void BootstrapMap(Dir* dir, addr_t where, addr_t physical)
		{
			Table* table = BootstrapCreateTable(dir, where);
						
			size_t pageid = GetPageId(where);
			table->page[pageid] = physical | TABLE_PRESENT | TABLE_WRITABLE;
		}

		void BootstrapMapStructures(Dir* dir)
		{
			// Map the dir itself.
			BootstrapMap(dir, (addr_t) kernelDir, (addr_t) dir);
			BootstrapMap(dir, (addr_t) currentDir, (addr_t) dir);

			// Map the tables.
			for ( size_t i = 0; i < ENTRIES; i++ )
			{
				addr_t tabledesc = dir->table[i];
				if ( tabledesc == 0 ) { continue; }
				addr_t mapto = (addr_t) &(currentTables[i]);
				addr_t mapfrom = (tabledesc & TABLE_ADDRESS);
				BootstrapMap(dir, mapto, mapfrom);
			}
		}
	#endif

	#ifdef PLATFORM_X86

		addr_t Lookup(addr_t where)
		{
			// Make sure we are only mapping kernel-approved pages.
			size_t tableid = GetTableId(where);
			addr_t tabledesc = currentDir->table[tableid];
			if ( !(tabledesc & DIR_PRESENT) ) { return 0; }

			size_t pageid = GetPageId(where);
			return currentTables[tableid].page[pageid];			
		}

		// Enables paging and flushes the Translation Lookaside Buffer (TLB).
		void Flush()
		{
			asm volatile("mov %0, %%cr3":: "r"(currentDirPhysical));
			size_t cr0; \
			asm volatile("mov %%cr0, %0": "=r"(cr0));
			cr0 |= 0x80000000UL; // Enable paging!
			asm volatile("mov %0, %%cr0":: "r"(cr0));
		}

		addr_t CreateAddressSpace()
		{
			return CreateDirectory();
		}

		addr_t SwitchAddressSpace(addr_t addrspace)
		{
			return SwitchDirectory(addrspace);
		}

		addr_t SwitchDirectory(addr_t dir)
		{
			// Don't switch if we are already there.
			if ( dir == currentDirPhysical ) { return currentDirPhysical; }

			addr_t previous = currentDirPhysical;
			asm volatile("mov %0, %%cr3":: "r"(dir));
			currentDirPhysical = dir;
			Flush();
			return previous;
		}

		addr_t CreateDirectory()
		{
			// Allocate the thread pages we need, one for the new pagedir,
			// and two for the last two 8 MiB of the pagedir.
			addr_t newdir = Page::Get();
			if ( newdir == 0 ) { return 0; }
			addr_t newstructmap1 = Page::Get();
			if ( newdir == 0 ) { Page::Put(newdir); return 0; }
			addr_t newstructmap2 = Page::Get();
			if ( newdir == 0 ) { Page::Put(newdir); Page::Put(newstructmap1); return 0; }

			// Map the new pagedir, clone the kernel dir, and change the last
			// 8 MiB, such that we can map the new page structures there.
			MapKernel((addr_t) makingDir, newdir);
			Memory::Copy(makingDir, kernelDir, sizeof(Dir));
			makingDir->table[1024-2] = newstructmap1 | DIR_PRESENT | DIR_WRITABLE;
			makingDir->table[1024-1] = newstructmap2 | DIR_PRESENT | DIR_WRITABLE;

			// Build the new page structures.
			MapKernel((addr_t) makingTable, newstructmap1);
			Memory::Set(makingTable, 0, sizeof(Table));
			makingTable->page[1024-2] = currentTables[1024-2].page[1024-2];
			makingTable->page[1024-1] = newdir | TABLE_PRESENT | TABLE_WRITABLE;	

			// Build the new page structures.
			MapKernel((addr_t) makingTable, newstructmap2);
			for ( size_t i = 0; i < 1024-2; i++ )
			{
				makingTable->page[i] = currentTables[1024-1].page[i];
			}
			makingTable->page[1024-2] = newstructmap1 | TABLE_PRESENT | TABLE_WRITABLE;
			makingTable->page[1024-1] = newstructmap2 | TABLE_PRESENT | TABLE_WRITABLE;

			return newdir;		
		}



		void MapKernel(addr_t where, addr_t physical)
		{
			// Make sure we are only mapping kernel-approved pages.
			size_t tableid = GetTableId(where);
			addr_t tabledesc = currentDir->table[tableid];
			ASSERT(tabledesc != 0);
			ASSERT((tabledesc & DIR_USER_SPACE) == 0);

			size_t pageid = GetPageId(where);
			addr_t pagedesc = physical | TABLE_PRESENT | TABLE_WRITABLE;
			currentTables[tableid].page[pageid] = pagedesc;

			ASSERT(Lookup(where) == pagedesc);

			// TODO: Only update the single page!
			Flush();
		}

		addr_t UnmapKernel(addr_t where)
		{
			// Make sure we are only unmapping kernel-approved pages.
			size_t tableid = GetTableId(where);
			addr_t tabledesc = currentDir->table[tableid];
			ASSERT(tabledesc != 0);
			ASSERT((tabledesc & DIR_USER_SPACE) == 0);

			size_t pageid = GetPageId(where);
			addr_t result = currentTables[tableid].page[pageid];
			ASSERT((result & TABLE_PRESENT) != 0);
			result &= TABLE_ADDRESS;
			currentTables[tableid].page[pageid] = 0;

			// TODO: Only update the single page!
			Flush();

			return result;
		}

		Table* CreateUserTable(addr_t where, bool maycreate)
		{
			size_t tableid = GetTableId(where);
			addr_t tabledesc = currentDir->table[tableid];

			Table* table = &(currentTables[tableid]);

			if ( tabledesc == 0 )
			{
				ASSERT(maycreate);
				addr_t tablepage = Page::Get();
				if ( tablepage == 0 ) { return NULL; }
				tabledesc = tablepage | TABLE_PRESENT | TABLE_WRITABLE | TABLE_USER_SPACE;
				currentDir->table[tableid] = tabledesc;
				MapKernel((addr_t) table, tablepage);

				// TODO: Only update the single page!
				Flush();

				addr_t lookup = Lookup((addr_t) table) & TABLE_ADDRESS;
				ASSERT(lookup == tablepage);

				Memory::Set(table, 0, sizeof(Table));
			}

			// Make sure we only touch dirs permitted for use by user-space!
			ASSERT((tabledesc & TABLE_USER_SPACE) != 0);

			return table;
		}

		bool MapUser(addr_t where, addr_t physical)
		{
			// Make sure we are only mapping user-space-approved pages.
			Table* table = CreateUserTable(where, true);
			if ( table == NULL ) { return false; }

			size_t pageid = GetPageId(where);
			addr_t pagedesc = physical | TABLE_PRESENT | TABLE_WRITABLE | TABLE_USER_SPACE;

			table->page[pageid] = pagedesc;

			Flush();

			ASSERT(Lookup(where) == pagedesc);

			// TODO: Only update the single page!
			Flush();

			return true;
		}

		addr_t UnmapUser(addr_t where)
		{
			// Make sure we are only mapping user-space-approved pages.
			Table* table = CreateUserTable(where, false);
			ASSERT(table != NULL);

			size_t pageid = GetPageId(where);
			addr_t pagedesc = table->page[pageid];
			ASSERT((pagedesc & TABLE_PRESENT) != 0);
			addr_t result = pagedesc & TABLE_ADDRESS;
			table->page[pageid] = 0;

			// TODO: Only update the single page!
			Flush();

			return result;
		}

		bool MapRangeKernel(addr_t where, size_t bytes)
		{
			for ( addr_t page = where; page < where + bytes; page += 4096UL )
			{
				addr_t physicalpage = Page::Get();
				if ( physicalpage == 0 )
				{
					while ( where < page )
					{
						page -= 4096UL;
						physicalpage = UnmapKernel(page);
						Page::Put(physicalpage);
					}
					return false;
				}

				MapKernel(page, physicalpage);
			}

			return true;
		}

		void UnmapRangeKernel(addr_t where, size_t bytes)
		{
			for ( addr_t page = where; page < where + bytes; page += 4096UL )
			{
				addr_t physicalpage = UnmapKernel(page);
				Page::Put(physicalpage);
			}
		}

		bool MapRangeUser(addr_t where, size_t bytes)
		{
			for ( addr_t page = where; page < where + bytes; page += 4096UL )
			{
				addr_t physicalpage = Page::Get();
				if ( physicalpage == 0 || !MapUser(page, physicalpage) )
				{
					while ( where < page )
					{
						page -= 4096UL;
						physicalpage = UnmapUser(page);
						Page::Put(physicalpage);
					}
					return false;
				}
			}

			return true;
		}

		void UnmapRangeUser(addr_t where, size_t bytes)
		{
			for ( addr_t page = where; page < where + bytes; page += 4096UL )
			{
				addr_t physicalpage = UnmapUser(page);
				Page::Put(physicalpage);
			}
		}

	#else

		#warning "Virtual Memory is not available on this arch"

		void Flush() { while(true); }
		void SwitchDirectory(addr_t dir) { while(true); }
		addr_t CreateDirectory() { while(true); return 0; }
		addr_t UnmapKernel(addr_t where) { while(true); return 0; }
		addr_t UnmapUser(addr_t where) { while(true); return 0; }

	#endif
	}
}

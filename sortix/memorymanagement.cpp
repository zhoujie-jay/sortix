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
#include "globals.h"
#include "iprintable.h"
#include "log.h"
#include "panic.h"
#include "multiboot.h"
#include "memorymanagement.h"

// Debug
#include "iirqhandler.h"
#include "keyboard.h"

using namespace Maxsi;

namespace Sortix
{
	const uintptr_t KernelStart = 0x000000;
	const size_t KernelLength = 0x200000;
	const size_t KernelLeadingPages = KernelLength / 0x1000;

	namespace Page
	{
		struct UnallocPage // Must like this due to assembly.
		{
			size_t Magic;
			void* Next;
			size_t ContinuousPages;
		};

		void Fragmentize();

		UnallocPage* volatile UnallocatedPage; // Must have this name and namespace due to assembly.
		size_t PagesTotal;
		//size_t PagesUsed;
		//size_t PagesFree;

		const size_t UnallocPageMagic = 0xABBAACDC; // Must this value due to assembly

		// Creates the Great Linked List of All Linked Lists!
		void Init(multiboot_info_t* BootInfo)
		{
			UnallocatedPage = NULL;
			PagesTotal = 0;

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

				Log::PrintF("RAM at 0x%64x\t of length 0x%64zx\n", MMap->addr, MMap->len);

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

					PagesTotal += Entries[I].Length;

					UnallocatedPage = Page;	
				}
			}

			if ( PagesTotal == 0 ) { Panic("memorymanagement.cpp: no RAM were available for paging"); }

			// Alright, time to make our linked list into a lot of small entries.
			// This speeds up the system when it's fully up and running. It only
			// takes a few miliseconds to run this operation on my laptop.
			Fragmentize();

#ifndef PLATFORM_SERIAL
			Log::PrintF("%zu pages are available for paging (%zu MiB RAM)\n", PagesTotal, PagesTotal >> 8 /* * 0x1000 / 1024 / 1024*/);
#endif
		}
	}

	namespace VirtualMemory
	{
	#ifdef PLATFORM_X86
		// These structures are always virtually mapped to these addresses.
		Dir*		CurrentDir		=	(Dir*)		0xFFBFF000;
		Table*		CurrentTables	=	(Table*)	0xFFC00000;
		uintptr_t	KernelHalfStart	=				0x80000000;
		uintptr_t	KernelHalfEnd	=				0xFFBFF000;
	#endif

	#ifdef PLATFORM_X64
		// TODO: These are dummy values!
		Dir*		CurrentDir		=	(Dir*)		0xFACEB00C;
		Table*		CurrentTables	=	(Table*)	0xFACEB00C;
		uintptr_t	KernelHalfStart	=				0xFACEB00C;
		uintptr_t	KernelHalfEnd	=				0xFACEB00C;
	#endif

		const size_t PointersPerPage = 4096 / sizeof(uintptr_t);

		size_t KernelLeadingPages;
		Dir* CurrentDirPhys;
		Dir* KernelDirPhys;
		bool Virgin;

		#define ENABLE_PAGING() \
		{ \
			size_t cr0; \
			asm volatile("mov %%cr0, %0": "=r"(cr0)); \
			cr0 |= 0x80000000UL; /* Enable paging! */ \
			asm volatile("mov %0, %%cr0":: "r"(cr0)); \
		}

		#define DISABLE_PAGING() \
		{ \
			size_t cr0; \
			asm volatile("mov %%cr0, %0": "=r"(cr0)); \
			cr0 &= ~(0x80000000UL); /* Disable paging! */ \
			asm volatile("mov %0, %%cr0":: "r"(cr0)); \
		}

		// Internally used functions
		void IdentityPhys(uintptr_t Addr);
		void FixupPhys(Dir* D);
		Table* GetTablePhys(uintptr_t Addr);

		void DebugTable(char* Base, Table* T)
		{
		#ifdef PLATFORM_X86
			Log::PrintF("-- Recursing to table at 0x%p spanning [0x%p - 0x%p] --\n", T, Base, Base + 1024 * 0x1000 - 1);
			for ( size_t I = 0; I < 1024; I++ )
			{
				uintptr_t Entry = T->Page[I];
				if ( Entry == 0 ) { continue; }
				Log::PrintF("[0x%p] -> [0x%p] [Flags=0x%x]\n", Base + I * 0x1000, Entry & TABLE_ADDRESS, Entry & TABLE_FLAGS);

				while ( true )
				{
					__asm__ ( "hlt" );
					//uint32_t Key = GKeyboard->HackGetKey();
					//if ( Key == '\n' ) { break; }
					//if ( Key == 0xFFFFFFFF - 25 ) { I += 23; break; }
				}
			}
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void DebugDir(Dir* D)
		{
		#ifdef PLATFORM_X86
			Log::PrintF("-- Start of debug of page dir at 0x%p --\n", D);
			if ( (uintptr_t) D & 1 ) { Log::PrintF("-- SOMETHING IS WRONG! --\n", D); while ( true ) { __asm__ ( "hlt" ); } }
			for ( size_t I = 0; I < 1024; I++ )
			{
				if ( !(D->Table[I] & DIR_PRESENT) ) { continue; }
				Table* T = (Table*) (D->Table[I] & DIR_ADDRESS);
				DebugTable((char*) (I * 0x40000), T);
			}
			Log::PrintF("-- End of debug of page dir at 0x%p --\n", D);
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void Init()
		{
		#ifdef PLATFORM_X86
			Virgin = true;

			// Allocate a page dir and reset it.
			CurrentDirPhys = (Dir*) Page::Get();
			if ( CurrentDirPhys == NULL ) { Panic("memorymanagement.cpp: Could not allocate page dir"); }
			Memory::Set(CurrentDirPhys, 0, sizeof(Dir));

			//while ( true ) { DebugDir(CurrentDirPhys); }

			// Identity map the kernel.
			for ( uintptr_t P = KernelStart; P < KernelStart + KernelLength; P += 0x1000 ) { IdentityPhys(P); }

			GetTablePhys(0x400000UL);
			GetTablePhys(0x80000000UL - 4096UL);

			// Initialize all the kernel tables from 0x8000000 to 0xFFFFFFFF here!
			for ( uintptr_t P = KernelHalfStart; P < KernelHalfEnd; P += 4096 ) { GetTablePhys(P); }

			// Prepare the page dir for real usage.
			FixupPhys(CurrentDirPhys);

			//while ( true ) { DebugDir(CurrentDirPhys); }

			// Now switch to the initial virtual address space.
			SwitchDir(CurrentDirPhys);

			// Remember this page dir as it is our base page dir.
			KernelDirPhys = CurrentDirPhys;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		Table* GetTablePhys(uintptr_t Addr)
		{
		#ifdef PLATFORM_X86
			// Find the desired table entry, if existing.
			uintptr_t DirIndex = Addr / 0x400000UL; // 4 MiB
			uintptr_t T = CurrentDirPhys->Table[DirIndex] & DIR_ADDRESS;

			// If the table doesn't exist, create it.
			if ( T == NULL )
			{
				// Allocate a page.
				T = (uintptr_t) Page::Get();
		
				// Check if anything went wrong.
				if ( T == NULL ) { Panic("memorymanagement.cpp: Could not allocate page table"); }

				// Reset the page's contents.
				Memory::Set((void*) T, 0, sizeof(Table));

				// Now add some flags
				uintptr_t Flags = DIR_PRESENT | DIR_WRITABLE | DIR_USER_SPACE;
				CurrentDirPhys->Table[DirIndex] = T | Flags;
			}

			return (Table*) T;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
			return NULL;
		#endif
		}

		void IdentityPhys(uintptr_t Addr)
		{
		#ifdef PLATFORM_X86
			Table* T = GetTablePhys(Addr);

			uintptr_t Flags = TABLE_PRESENT | TABLE_WRITABLE;

			size_t TableIndex = (Addr % 0x400000) / 0x1000;
			T->Page[TableIndex] = Addr | Flags;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void FixupPhys(Dir* D)
		{
		#ifdef PLATFORM_X86
			Table* SecondLastTable = GetTablePhys((uintptr_t) CurrentDir);

			uintptr_t Flags = TABLE_PRESENT | TABLE_WRITABLE;
			uintptr_t DirEntry = ((uintptr_t) D) | Flags;

			SecondLastTable->Page[PointersPerPage-1] = DirEntry;

			Table* LastTable = GetTablePhys((uintptr_t) CurrentTables);
	
			for ( size_t I = 0; I < PointersPerPage; I++ )
			{
				LastTable->Page[I] = (D->Table[I] & DIR_ADDRESS) | Flags;
			}
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void Fixup(Dir* D)
		{
		#ifdef PLATFORM_X86
			uintptr_t Flags = TABLE_PRESENT | TABLE_WRITABLE;

			Table* T = &CurrentTables[PointersPerPage-1];
	
			for ( size_t I = 0; I < PointersPerPage; I++ )
			{
				T->Page[I] = (D->Table[I] & DIR_ADDRESS) | Flags;
			}
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void SwitchDir(Dir* PhysicalDirAddr)
		{
		#ifdef PLATFORM_X86
			// Set the new page directory.
			CurrentDirPhys = PhysicalDirAddr;
			asm volatile("mov %0, %%cr3":: "r"(PhysicalDirAddr));

			if ( !Virgin )
			{
				uintptr_t Entry = ((uintptr_t) PhysicalDirAddr & DIR_ADDRESS) | TABLE_PRESENT | TABLE_WRITABLE;
				CurrentTables[PointersPerPage-2].Page[PointersPerPage-1] = Entry;
			}

			// Reset the paging flag in the cr0 register to enable paging, and flush the paging cache.
			ENABLE_PAGING();

			Virgin = false;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void Flush()
		{
		#ifdef PLATFORM_X86
			Fixup(CurrentDir);

			ENABLE_PAGING();
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		Dir* NewDir()
		{
		#ifdef PLATFORM_X86
			DISABLE_PAGING();

			// TODO: Is the stack well defined here?!
			Dir* Result = (Dir*) Page::Get();

			if ( Result != NULL )
			{
				Memory::Copy(Result, KernelDirPhys, sizeof(Dir));
			}

			ENABLE_PAGING();

			return Result;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
			return NULL;
		#endif
		}

		void Map(uintptr_t Physical, uintptr_t Virtual, uintptr_t Flags)
		{
		#ifdef PLATFORM_X86
			// TODO: Possibly validate Physical and Virtual are aligned, and that
			// flags uses only legal bits. Should the function then Panic?
	
			size_t DirIndex = Virtual / 0x400000; // 4 MiB

			// See if the required table in the dir exists.
			if ( !(CurrentDir->Table[DirIndex] & DIR_PRESENT) )
			{
				Log::PrintF("3-1-1\n");
				//DISABLE_PAGING();
				// TODO: Is the stack well defined here?!

				// This will create the table we need.
				GetTablePhys(Virtual);

				//ENABLE_PAGING();
			}

			size_t TableIndex = (Virtual % 0x400000) / 0x1000;		
		
			CurrentTables[DirIndex].Page[TableIndex] = Physical | Flags;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		// Unsafe version of VirtualMemory::Map. This has no error checking, but is faster.
		void MapUnsafe(uintptr_t Physical, uintptr_t Virtual, uintptr_t Flags)
		{
		#ifdef PLATFORM_X86
			size_t DirIndex = Virtual / 0x400000; // 4 MiB
			size_t TableIndex = (Virtual % 0x400000) / 0x1000;		
		
			CurrentTables[DirIndex].Page[TableIndex] = Physical | Flags;
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
		#endif
		}

		void* LookupAddr(uintptr_t Virtual)
		{
		#ifdef PLATFORM_X86
			size_t DirIndex = Virtual / 0x400000; // 4 MiB
			size_t TableIndex = (Virtual % 0x400000) / 0x1000;		
		
			return (void*) (CurrentTables[DirIndex].Page[TableIndex] & TABLE_ADDRESS);
		#else
			#warning "Virtual Memory is not available on this arch"
			while(true);
			return NULL;
		#endif
		}
	}
}

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

namespace Sortix
{
	namespace Page
	{
#ifdef MULTIBOOT_HEADER
		void	Init(multiboot_info_t* BootInfo);
#endif
		void*	Get();
		void	Put(void* Page);
		void	GetNoCache();
		void	PutNoCache(void* Page);
	}

	namespace VirtualMemory
	{
		// TODO: Convert these to constants!
		#define TABLE_PRESENT		(1<<0)
		#define TABLE_WRITABLE		(1<<1)
		#define TABLE_USER_SPACE	(1<<2)
		#define TABLE_RESERVED1		(1<<3) // Used internally by the CPU.
		#define TABLE_RESERVED2		(1<<4) // Used internally by the CPU.
		#define TABLE_ACCESSED		(1<<5)
		#define TABLE_DIRTY			(1<<6)
		#define TABLE_RESERVED3		(1<<7) // Used internally by the CPU.
		#define TABLE_RESERVED4		(1<<8) // Used internally by the CPU.
		#define TABLE_AVAILABLE1	(1<<9)
		#define TABLE_AVAILABLE2	(1<<10)
		#define TABLE_AVAILABLE3	(1<<11)
		#define TABLE_FLAGS			(0xFFFUL) // Bits used for the flags.
		#define TABLE_ADDRESS		(~0xFFFUL) // Bits used for the address.

		#define DIR_PRESENT			(1<<0)
		#define DIR_WRITABLE		(1<<1)
		#define DIR_USER_SPACE		(1<<2)
		#define DIR_WRITE_THROUGH	(1<<3)
		#define DIR_DISABLE_CACHE	(1<<4)
		#define DIR_ACCESSED		(1<<5)
		#define DIR_RESERVED1		(1<<6)
		#define DIR_4MIB_PAGES		(1<<7)
		#define DIR_RESERVED2		(1<<8)
		#define DIR_AVAILABLE1		(1<<9)
		#define DIR_AVAILABLE2		(1<<10)
		#define DIR_AVAILABLE3		(1<<11)
		#define DIR_FLAGS			(0xFFFUL) // Bits used for the flags.
		#define DIR_ADDRESS			(~0xFFFUL) // Bits used for the address.

		struct Table
		{
			uintptr_t Page[4096 / sizeof(uintptr_t)];
		};

		struct Dir
		{
			uintptr_t Table[4096 / sizeof(uintptr_t*)];
		};

		void	Init();
		void	SwitchDir(Dir* PhysicalDirAddr);
		void	Map(uintptr_t Physical, uintptr_t Virtual, uintptr_t Flags);
		void	MapUnsafe(uintptr_t Physical, uintptr_t Virtual, uintptr_t Flags);
		void*	LookupAddr(uintptr_t Virtual);
		void	Flush();
		void	Fixup(Dir* Dir);
		Dir*	NewDir();
	}

	bool ValidateUserString(const char* USER string);
}

#endif


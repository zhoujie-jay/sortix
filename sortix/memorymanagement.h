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
		addr_t	Get();
		void	Put(addr_t Page);
	}

	namespace VirtualMemory
	{
		void Init();
		void Flush();
		addr_t CreateAddressSpace();
		addr_t SwitchAddressSpace(addr_t addrspace);
		void MapKernel(addr_t where, addr_t physical);
		bool MapUser(addr_t where, addr_t physical);
		addr_t UnmapKernel(addr_t where);
		addr_t UnmapUser(addr_t where);

#ifdef PLATFORM_X86
		const addr_t heapLower = 0x80000000UL;
		const addr_t heapUpper = 0xFF800000UL;
#endif

	}
}

#endif


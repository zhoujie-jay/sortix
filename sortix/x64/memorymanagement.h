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
	Handles memory for the x64 architecture.

******************************************************************************/

#ifndef SORTIX_X64_MEMORYMANAGEMENT_H
#define SORTIX_X64_MEMORYMANAGEMENT_H

namespace Sortix
{
	namespace Memory
	{
		const size_t TOPPMLLEVEL = 4;
		const size_t ENTRIES = 4096UL / sizeof(addr_t);
		const size_t TRANSBITS = 9;

		PML* const PMLS[TOPPMLLEVEL + 1] =
		{
			(PML* const) 0x0,
			(PML* const) 0xFFFFFF8000000000UL,
			(PML* const) 0xFFFFFF7FC0000000UL,
			(PML* const) 0XFFFFFF7FBFE00000UL,
			(PML* const) 0xFFFFFF7FBFDFF000UL,
		};

		PML* const FORKPML = (PML* const) 0xFFFFFF0000000000UL;
	}

	namespace Page
	{
		addr_t* const STACK = (addr_t* const) 0xFFFFFE8000000000UL;
		const size_t MAXSTACKSIZE = (512UL*1024UL*1024UL*1024UL);
		const size_t MAXSTACKLENGTH = MAXSTACKSIZE / sizeof(addr_t);
	}
}

#endif

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    x86/memorymanagement.h
    Handles memory for the x64 architecture.

*******************************************************************************/

#ifndef SORTIX_X64_MEMORYMANAGEMENT_H
#define SORTIX_X64_MEMORYMANAGEMENT_H

namespace Sortix {
namespace Memory {

const size_t TOPPMLLEVEL = 2;
const size_t ENTRIES = 4096UL / sizeof(addr_t);
const size_t TRANSBITS = 10;

PML* const PMLS[TOPPMLLEVEL + 1] =
{
	(PML* const) 0x0,
	(PML* const) 0xFFC00000UL,
	(PML* const) 0xFFBFF000UL,
};

PML* const FORKPML = (PML* const) 0xFF800000UL;

} // namespace Memory
} // namespace Sortix

namespace Sortix {
namespace Page {

addr_t* const STACK = (addr_t* const) 0xFF400000UL;
const size_t MAXSTACKSIZE = (4UL*1024UL*1024UL);
const size_t MAXSTACKLENGTH = MAXSTACKSIZE / sizeof(addr_t);

} // namespace Page
} // namespace Sortix

#endif

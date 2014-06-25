/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    sortix/kernel/elf.h
    Executable and Linkable Format.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_ELF_H
#define INCLUDE_SORTIX_KERNEL_ELF_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {
namespace ELF {

struct Auxiliary
{
	size_t tls_file_offset;
	size_t tls_file_size;
	size_t tls_mem_size;
	size_t tls_mem_align;
	size_t uthread_size;
	size_t uthread_align;
};

// Reads the elf file into the current address space and returns the entry
// address of the program, or 0 upon failure.
uintptr_t Load(const void* file, size_t filelen, Auxiliary* aux);

} // namespace ELF
} // namespace Sortix

#endif

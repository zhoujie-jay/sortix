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

    sortix/kernel/cpu.h
    CPU-specific declarations.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_CPU_H
#define INCLUDE_SORTIX_KERNEL_CPU_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/decl.h>

namespace Sortix {

// Functions for 32-bit and 64-bit x86.
#if defined(__i386__) || defined(__x86_64__)
namespace CPU {
void Reboot();
void ShutDown();
} // namespace CPU
#endif

} // namespace Sortix

#endif

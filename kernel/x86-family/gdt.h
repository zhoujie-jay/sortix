/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    x86-family/gdt.h
    Initializes and handles the GDT and TSS.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_GDT_H
#define SORTIX_X86_FAMILY_GDT_H

#include <stdint.h>

namespace Sortix {
namespace GDT {

void Init();
void WriteTSS(int32_t num, uint16_t ss0, uintptr_t stack0);
uintptr_t GetKernelStack();
void SetKernelStack(uintptr_t stack_pointer);
#if defined(__i386__)
uint32_t GetFSBase();
uint32_t GetGSBase();
void SetFSBase(uint32_t fsbase);
void SetGSBase(uint32_t gsbase);
#endif

} // namespace GDT
} // namespace Sortix

#endif

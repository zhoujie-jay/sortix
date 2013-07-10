/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    cpu.h
    Includes CPU-specific headers.

*******************************************************************************/

#ifndef SORTIX_CPU_H
#define SORTIX_CPU_H

	// Include some x86 headers.
	#ifdef PLATFORM_X86
		#include "x86/x86.h"
	#endif

	// Include some x64 headers.
	#ifdef PLATFORM_X64
		#include "x64/x64.h"
	#endif

namespace Sortix {
namespace CPU {

extern "C" void load_registers(InterruptRegisters* regs, size_t size) SORTIX_NORETURN;
inline void LoadRegisters(InterruptRegisters* regs) SORTIX_NORETURN;
inline void LoadRegisters(InterruptRegisters* regs)
{
	load_registers(regs, sizeof(*regs));
}

} // namespace CPU
} // namespace CPU

#endif

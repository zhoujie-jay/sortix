/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    x86-family/float.h
    Handles saving and restoration of floating point numbers.

*******************************************************************************/

#ifndef SORTIX_FLOAT_H
#define SORTIX_FLOAT_H

namespace Sortix {

class Thread;

namespace Float {

extern bool fpu_is_enabled;

void Init();
void NofityTaskExit(Thread* thread);
void Yield();

static inline void EnableFPU()
{
	asm volatile ("clts");
	fpu_is_enabled = true;
}

static inline void DisableFPU()
{
	fpu_is_enabled = false;
	unsigned long cr0;
	asm volatile ("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 1UL<<3UL;
	asm volatile ("mov %0, %%cr0" : : "r"(cr0));
}

static inline void NotityTaskSwitch()
{
	DisableFPU();
}

} // namespace Float

} // namespace Sortix
#endif

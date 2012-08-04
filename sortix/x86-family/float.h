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

	float.cpp
	Handles saving and restoration of floating point numbers.

*******************************************************************************/

#ifndef SORTIX_FLOAT_H
#define SORTIX_FLOAT_H

namespace Sortix {
namespace Float {

void Init();
void NofityTaskExit();

static inline void EnableFPU()
{
	asm volatile ("clts");
}

static inline void DisableFPU()
{
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

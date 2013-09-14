/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    x86-family/pat.cpp
    Tests whether PAT is available and initializes it.

*******************************************************************************/

#include <msr.h>
#include <stdint.h>

#include <sortix/kernel/cpuid.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/pat.h>

#include "memorymanagement.h"

namespace Sortix {

static const uint32_t bit_PAT = 0x00010000U;

bool IsPATSupported()
{
	if ( !IsCPUIdSupported() )
		return false;
	uint32_t eax, ebx, ecx, edx;
	cpuid(1, eax, ebx, ecx, edx);
	uint32_t features = edx;
	return features & bit_PAT;
}

void InitializePAT()
{
	using namespace Sortix::Memory;
	const uint32_t LOW = PA[0] << 0 | PA[1] << 8 | PA[2] << 16 | PA[3] << 24;
	const uint32_t HIGH = PA[4] << 0 | PA[5] << 8 | PA[6] << 16 | PA[7] << 24;
	const int PAT_REG = 0x0277;
	wrmsr(PAT_REG, (uint64_t) LOW << 0 | (uint64_t) HIGH << 32);
}

} // namespace Sortix

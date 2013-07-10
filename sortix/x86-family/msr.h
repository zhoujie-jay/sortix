/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    x86-family/msr.h
    Functions to manipulate Model Specific Registers.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_MSR_H
#define SORTIX_X86_FAMILY_MSR_H

namespace Sortix {
namespace MSR {

bool IsPATSupported();
void InitializePAT();
bool IsMTRRSupported();
const char* SetupMTRRForWC(addr_t base, size_t size, int* ret = NULL);
void EnableMTRR(int mtrr);
void DisableMTRR(int mtrr);
void CopyMTRR(int dst, int src);

} // namespace MSR
} // namespace Sortix

#endif

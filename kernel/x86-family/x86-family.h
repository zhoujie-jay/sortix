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

    x86-family/x86-family.h
    CPU stuff for the x86 CPU family.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_X86_FAMILY_H
#define SORTIX_X86_FAMILY_X86_FAMILY_H

#include <sortix/kernel/decl.h>

namespace Sortix {
namespace CPU {

void Init();

} // namespace CPU
} // namespace Sortix

#endif

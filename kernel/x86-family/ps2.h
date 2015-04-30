/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    x86-family/ps2.h
    8042 PS/2 Controller.

*******************************************************************************/

#ifndef SORTIX_X86_FAMILY_PS2_H
#define SORTIX_X86_FAMILY_PS2_H

#include <stdint.h>

#include <sortix/kernel/ps2.h>

namespace Sortix {
namespace PS2 {

void Init(PS2Device* keyboard, PS2Device* mouse);

} // namespace PS2
} // namespace Sortix

#endif

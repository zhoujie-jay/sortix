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

    x86-family/float.cpp
    Handles saving and restoration of floating point numbers.

*******************************************************************************/

#include <assert.h>

#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/thread.h>

#include "float.h"

namespace Sortix {
namespace Float {

extern "C" { __attribute__((aligned(16))) uint8_t fpu_initialized_regs[512]; }

} // namespace Float
} // namespace Sortix

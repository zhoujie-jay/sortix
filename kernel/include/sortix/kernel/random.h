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

    sortix/kernel/random.h
    Kernel entropy gathering.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_RANDOM_H
#define INCLUDE_SORTIX_KERNEL_RANDOM_H

#include <stddef.h>

namespace Sortix {
namespace Random {

bool HasEntropy();
void GetEntropy(void* buffer, size_t size);

} // namespace Random
} // namespace Sortix

#endif

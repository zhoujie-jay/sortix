/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/kernel/sockopt.h
    getsockopt() and setsockopt() utility functions.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_SOCKOPT_H
#define INCLUDE_SORTIX_KERNEL_SOCKOPT_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/ioctx.h>

namespace Sortix {

bool sockopt_fetch_uintmax(uintmax_t* optval_ptr, ioctx_t* ctx,
                           const void* option_value, size_t option_size);
bool sockopt_return_uintmax(uintmax_t optval, ioctx_t* ctx,
                            void* option_value, size_t* option_size_ptr);

} // namespace Sortix

#endif

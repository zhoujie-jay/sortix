/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    sortix/kernel/copy.h
    The context for io operations: who made it, how should data be copied, etc.

*******************************************************************************/

#ifndef SORTIX_COPY_H
#define SORTIX_COPY_H

#include <stddef.h>

namespace Sortix {

bool CopyToUser(void* userdst, const void* ksrc, size_t count);
bool CopyFromUser(void* kdst, const void* usersrc, size_t count);
bool CopyToKernel(void* kdst, const void* ksrc, size_t count);
bool CopyFromKernel(void* kdst, const void* ksrc, size_t count);
bool ZeroKernel(void* kdst, size_t count);
bool ZeroUser(void* userdst, size_t count);
char* GetStringFromUser(const char* str);

} // namespace Sortix

#endif

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

    sortix/kernel/string.h
    Useful functions for manipulating strings that don't belong in libc.

*******************************************************************************/

#ifndef SORTIX_STRING_H
#define SORTIX_STRING_H

#include <stddef.h>

namespace Sortix {
namespace String {

char* Clone(const char* Source);
char* Substring(const char* src, size_t offset, size_t length);
bool StartsWith(const char* Haystack, const char* Needle);

} // namespace String
} // namespace Sortix

#endif

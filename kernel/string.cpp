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

    string.cpp
    Useful functions for manipulating strings that don't belong in libc.

*******************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <sortix/kernel/string.h>

namespace Sortix {
namespace String {

bool StartsWith(const char* haystack, const char* needle)
{
	return strncmp(haystack, needle, strlen(needle)) == 0;
}

char* Clone(const char* Input)
{
	size_t InputSize = strlen(Input);
	char* Result = new char[InputSize + 1];
	if ( Result == NULL ) { return NULL; }
	memcpy(Result, Input, InputSize + 1);
	return Result;
}

char* Substring(const char* src, size_t offset, size_t length)
{
	char* dest = new char[length + 1];
	if ( !dest ) { return NULL; }
	memcpy(dest, src + offset, length * sizeof(char));
	dest[length] = 0;
	return dest;
}

} // namespace String
} // namespace Sortix

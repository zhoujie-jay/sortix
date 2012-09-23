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
	size_t srclen = strlen(src);
	char* dest = new char[length + 1];
	if ( !dest ) { return NULL; }
	memcpy(dest, src + offset, length * sizeof(char));
	dest[length] = 0;
	return dest;
}

char* Combine(size_t NumParameters, ...)
{
	va_list param_pt;

	va_start(param_pt, NumParameters);

	// First calculate the string length.
	size_t ResultLength = 0;
	const char* TMP = 0;

	for ( size_t I = 0; I < NumParameters; I++ )
	{
		TMP = va_arg(param_pt, const char*);

		if ( TMP != NULL ) { ResultLength += strlen(TMP); }
	}

	// Allocate a string with the desired length.
	char* Result = new char[ResultLength+1];
	if ( !Result ) { return NULL; }

	Result[0] = 0;

	va_end(param_pt);
	va_start(param_pt, NumParameters);

	size_t ResultOffset = 0;

	for ( size_t I = 0; I < NumParameters; I++ )
	{
		TMP = va_arg(param_pt, const char*);

		if ( TMP )
		{
			size_t TMPLength = strlen(TMP);
			strcpy(Result + ResultOffset, TMP);
			ResultOffset += TMPLength;
		}
	}

	return Result;
}

} // namespace String
} // namespace Sortix

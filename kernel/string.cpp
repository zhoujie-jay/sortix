/*
 * Copyright (c) 2011, 2012 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * string.cpp
 * Useful functions for manipulating strings that don't belong in libc.
 */

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

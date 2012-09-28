/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of the Sortix C Library.

	The Sortix C Library is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or (at your
	option) any later version.

	The Sortix C Library is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
	License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

	sprint.cpp
	Provides the stubs for the sprintf family of functions.

*******************************************************************************/

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct vsnprintf_struct
{
	char* str;
	size_t size;
	size_t produced;
	size_t written;
} vsnprintf_t;

static size_t StringPrintCallback(void* user, const char* string, size_t stringlen)
{
	vsnprintf_t* info = (vsnprintf_t*) user;
	if ( info->produced < info->size )
	{
		size_t available = info->size - info->produced;
		size_t possible = (stringlen < available) ? stringlen : available;
		memcpy(info->str + info->produced, string, possible);
		info->written += possible;
	}
	info->produced += stringlen;
	return stringlen;
}

extern "C" int vsnprintf(char* restrict str, size_t size, const char* restrict format, va_list list)
{
	vsnprintf_t info;
	info.str = str;
	info.size = (size) ? size-1 : 0;
	info.produced = 0;
	info.written = 0;
	vprintf_callback(StringPrintCallback, &info, format, list);
	if ( size ) { info.str[info.written] = '\0'; }
	return (int) info.produced;
}

extern "C" int snprintf(char* restrict str, size_t size, const char* restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vsnprintf(str, size, format, list);
	va_end(list);
	return result;
}

extern "C" int vsprintf(char* restrict str, const char* restrict format, va_list list)
{
	return vsnprintf(str, SIZE_MAX, format, list);
}

extern "C" int sprintf(char* restrict str, const char* restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vsprintf(str, format, list);
	va_end(list);
	return result;
}

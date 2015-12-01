/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    stdio/vsnprintf.cpp
    Prints a formatted string to the supplied buffer.

*******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct vsnprintf
{
	char* str;
	size_t size;
	size_t written;
};

static size_t vsnprintf_callback(void* ctx, const char* string, size_t length)
{
	struct vsnprintf* info = (struct vsnprintf*) ctx;
	if ( 1 <= info->size && info->written < info->size )
	{
		size_t available = info->size - info->written;
		size_t possible = length < available ? length : available;
		memcpy(info->str + info->written, string, possible);
		info->written += possible;
	}
	return length;
}

extern "C"
int vsnprintf(char* restrict str,
              size_t size,
              const char* restrict format,
              va_list list)
{
	struct vsnprintf info;
	info.str = str;
	info.size = size ? size - 1 : 0;
	info.written = 0;
	int result = vcbprintf(&info, vsnprintf_callback, format, list);
	if ( 1 <= size )
		info.str[info.written] = '\0';
	return result;
}

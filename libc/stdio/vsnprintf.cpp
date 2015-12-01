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
	struct vsnprintf* state = (struct vsnprintf*) ctx;
	if ( 1 <= state->size && state->written < state->size )
	{
		size_t available = state->size - state->written;
		size_t possible = length < available ? length : available;
		memcpy(state->str + state->written, string, possible);
		state->written += possible;
	}
	return length;
}

extern "C"
int vsnprintf(char* restrict str,
              size_t size,
              const char* restrict format,
              va_list list)
{
	struct vsnprintf state;
	state.str = str;
	state.size = size ? size - 1 : 0;
	state.written = 0;
	int result = vcbprintf(&state, vsnprintf_callback, format, list);
	if ( 1 <= size )
		state.str[state.written] = '\0';
	return result;
}

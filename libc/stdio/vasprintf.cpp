/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    stdio/vasprintf.cpp
    Prints a string to a newly allocated buffer.

*******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct vasprintf
{
	char* buffer;
	size_t used;
	size_t size;
};

static size_t vasprintf_callback(void* ctx, const char* string, size_t length)
{
	struct vasprintf* state = (struct vasprintf*) ctx;
	size_t needed_size = state->used + length + 1;
	if ( state->size < needed_size )
	{
		// TODO: Overflow check.
		size_t new_size = 2 * state->size;
		if ( new_size < needed_size )
			new_size = needed_size;
		char* new_buffer = (char*) realloc(state->buffer, new_size);
		if ( !new_buffer )
		{
			free(state->buffer);
			state->buffer = NULL;
			return 0;
		}
		state->buffer = new_buffer;
		state->size = new_size;
	}
	memcpy(state->buffer + state->used, string, length);
	state->used += length;
	return length;
}

extern "C"
int vasprintf(char** restrict result_ptr,
              const char* restrict format,
              va_list list)
{
	struct vasprintf state;
	state.used = 0;
	state.size = 32;
	if ( !(state.buffer = (char*) malloc(state.size)) )
		return -1;
	int result = vcbprintf(&state, vasprintf_callback, format, list);
	if ( !state.buffer )
		return -1;
	state.buffer[state.used] = '\0';
	return *result_ptr = state.buffer, result;
}

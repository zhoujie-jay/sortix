/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

struct vasprintf_state
{
	char* string;
	size_t string_length;
	size_t string_used;
};

static size_t vasprintf_callback(void* user, const char* string, size_t length)
{
	struct vasprintf_state* state = (struct vasprintf_state*) user;
	if ( !state->string )
		return 0;
	size_t needed_length = state->string_used + length + 1;
	if ( state->string_length < needed_length )
	{
		size_t new_length = 2 * state->string_used;
		if ( new_length < needed_length )
			new_length = needed_length;
		size_t new_size = new_length * sizeof(char);
		char* new_string = (char*) realloc(state->string, new_size);
		if ( !new_string )
		{
			free(state->string);
			state->string = NULL;
			return 0;
		}
		state->string = new_string;
		state->string_length = new_length;
	}
	memcpy(state->string + state->string_used, string, sizeof(char) * length);
	state->string_used += length;
	return length;
}

extern "C"
int vasprintf(char** restrict result_ptr,
              const char* restrict format,
              va_list list)
{
	const size_t DEFAULT_SIZE = 32;
	struct vasprintf_state state;
	state.string_length = DEFAULT_SIZE;
	state.string_used = 0;
	if ( !(state.string = (char*) malloc(state.string_length * sizeof(char))) )
		return *result_ptr = NULL, -1;
	vprintf_callback(vasprintf_callback, &state, format, list);
	if ( !state.string )
		return *result_ptr = NULL, -1;
	state.string[state.string_used] = '\0';
	return *result_ptr = state.string, (int) state.string_used;
}

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    util.c
    Utility functions.

*******************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

char* strdup_safe(const char* string)
{
	return string ? strdup(string) : NULL;
}

const char* getenv_safe_def(const char* name, const char* def)
{
	const char* ret = getenv(name);
	return ret ? ret : def;
}

const char* getenv_safe(const char* name)
{
	return getenv_safe_def(name, "");
}

bool array_add(void*** array_ptr,
               size_t* used_ptr,
               size_t* length_ptr,
               void* value)
{
	void** array;
	memcpy(&array, array_ptr, sizeof(array)); // Strict aliasing.

	if ( *used_ptr == *length_ptr )
	{
		// TODO: Avoid overflow.
		size_t new_length = 2 * *length_ptr;
		if ( !new_length )
			new_length = 16;
		// TODO: Avoid overflow and use reallocarray.
		size_t new_size = new_length * sizeof(void*);
		void** new_array = (void**) realloc(array, new_size);
		if ( !new_array )
			return false;
		array = new_array;
		memcpy(array_ptr, &array, sizeof(array)); // Strict aliasing.
		*length_ptr = new_length;
	}

	memcpy(array + (*used_ptr)++, &value, sizeof(value)); // Strict aliasing.

	return true;
}

void stringbuf_begin(struct stringbuf* buf)
{
	buf->length = 0;
	buf->allocated_size = 32;
	if ( !(buf->string = (char*) malloc(buf->allocated_size)) )
	{
		buf->allocated_size = 0;
		return;
	}
	buf->string[0] = '\0';
}

bool might_need_shell_quote(char c)
{
	switch ( c )
	{
	case '|':
	case '&':
	case ';':
	case '<':
	case '>':
	case '(':
	case ')':
	case '$':
	case '`':
	case '\\':
	case '"':
	case '\'':
	case ' ':
	case '\t':
	case '*':
	case '?':
	case '[':
	case '#':
	case '~':
	case '=':
	case '%':
		return true;
	default:
		return false;
	}
}

void stringbuf_append_c(struct stringbuf* buf, char c)
{
	if ( !buf->string )
		return;
	if ( buf->length + 1 == buf->allocated_size )
	{
		// TODO: Avoid overflow.
		size_t new_allocated_size = 2 * buf->allocated_size;
		char* new_string = (char*) realloc(buf->string, new_allocated_size);
		if ( !new_string )
		{
			buf->string = NULL;
			buf->length = 0;
			buf->allocated_size = 0;
			return;
		}
		buf->string = new_string;
		buf->allocated_size = new_allocated_size;
	}
	buf->string[buf->length++] = c;
	buf->string[buf->length] = '\0';
}

char* stringbuf_finish(struct stringbuf* buf)
{
	if ( !buf->string )
		return NULL;
	return buf->string;
}

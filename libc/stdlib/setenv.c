/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * stdlib/setenv.c
 * Set an environment variable.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline
bool matches_environment_variable(const char* what,
                                  const char* name, size_t name_length)
{
	return !strncmp(what, name, name_length) && what[name_length] == '=';
}

static char* create_entry(const char* name, size_t name_length,
                          const char* value, size_t value_length)
{
	size_t result_length = name_length + 1 + value_length;
	size_t result_size = sizeof(char) * (result_length + 1);
	char* result = (char*) malloc(result_size);
	if ( !result )
		return NULL;
	memcpy(result, name, name_length);
	result[name_length] = '=';
	memcpy(result + name_length + 1, value, value_length);
	result[name_length + 1 + value_length] = '\0';
	return result;
}

static bool set_entry_at(char** environ, size_t position,
                         const char* name, size_t name_length,
                         const char* value, size_t value_length)
{
	char* new_entry = create_entry(name, name_length, value, value_length);
	if ( !new_entry)
		return false;
	free(environ[position]);
	environ[position] = new_entry;
	return true;
}

static bool recover_environment(void)
{
	// It is undefined behavior to change environ to another value (manually
	// taking control of the environment) and reusing any of the pointers from
	// the former environ array. A program that assigns to environ must use its
	// own memory for the environ array and all the strings pointed to by it.
	// TODO: Is that actually undefined behavior?

	// Destroy the old environ array and its entries that we'll assume is no
	// longer used as that would be undefined behavior as said above.
	if ( __environ_malloced )
	{
		for ( size_t i = 0; i < __environ_used; i++ )
			free(__environ_malloced[i]);
		free(__environ_malloced);
		__environ_malloced = NULL;
		__environ_length = 0;
		__environ_used = 0;
	}

	// Prepare a new environ array into which we'll store all the strings.
	size_t environ_length = 0;
	if ( environ )
		while ( environ[environ_length] )
			environ_length++;
	size_t environ_size = sizeof(char*) * (environ_length + 1);
	char** environ_malloced = (char**) malloc(environ_size);
	if ( !environ_malloced )
		return false;

	// Duplicate all the environment variables into the new environment.
	for ( size_t i = 0; i < environ_length; i++ )
	{
		if ( !(environ_malloced[i] = strdup(environ[i])) )
		{
			for ( size_t n = 0; n < i; n++ )
				free(environ_malloced[n]);
			free(environ_malloced);
			return false;
		}
	}
	environ_malloced[environ_length] = NULL;

	// Switch to the newly recovered environment.
	environ = __environ_malloced = environ_malloced;
	__environ_used = environ_length;
	__environ_length = environ_length;

	return true;
}

int setenv(const char* name, const char* value, int overwrite)
{
	if ( !name || !value )
		return errno = EINVAL, -1;

	if ( !name[0] )
		return errno = EINVAL, -1;

	// Verify the name doesn't contain a '=' character.
	size_t name_length = 0;
	while ( name[name_length] )
		if ( name[name_length++] == '=' )
			return errno = EINVAL, -1;

	// Take back control of the environment if the user changed the environ
	// pointer to something the user controls or if the environment has not been
	// initialized yet besides the read-only initial environment.
	if ( (!environ || environ != __environ_malloced) && !recover_environment() )
		return -1;

	size_t value_length = strlen(value);

	// Attempt to locate an existing environment variable with this name.
	for ( size_t i = 0; i < __environ_used; i++ )
	{
		char* previous_entry = environ[i];
		if ( !matches_environment_variable(previous_entry, name, name_length) )
			continue;

		// Report success if the caller doesn't want us to change an existing
		// environment variable, but still set the variable if not set yet.
		if ( !overwrite )
			return 0;

		// Avoid the need to allocate a new string if there is room in the old
		// allocation for the contents of the new string.
		char* previous_value = previous_entry + name_length + 1;
		size_t previous_value_length = strlen(previous_value);
		if ( value_length <= previous_value_length )
		{
			strcpy(previous_value, value);
			return 0;
		}

		// Insert the new entry in the environ array at the same location.
		bool result = set_entry_at(environ, i, name, name_length, value, value_length);
		return result ? 0 : -1;
	}

	// Expand the environ array if it is currently fully used such that we have
	// room to insert the new entry below.
	if ( __environ_used == __environ_length )
	{
		size_t new_length = __environ_length ? 2 * __environ_length : 16;
		size_t new_size = sizeof(char*) * (new_length + 1);
		char** new_environ = (char**) realloc(environ, new_size);
		if ( !new_environ )
			return -1;
		environ = __environ_malloced = new_environ;
		for ( size_t i = __environ_used; i <= new_length; i++ )
			environ[i] = NULL;
		__environ_length = new_length;
	}

	// Append the new entry to the environ array.
	if ( !set_entry_at(environ, __environ_used,
	                   name, name_length,
	                   value, value_length) )
		return -1;
	return __environ_used++, 0;
}

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
 * stdlib/unsetenv.c
 * Unset an environment variable.
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

int unsetenv(const char* name)
{
	if ( !name )
		return errno = EINVAL, -1;

	if ( !name[0] )
		return errno = EINVAL, -1;

	// Verify the name doesn't contain a '=' character.
	size_t name_length = 0;
	while ( name[name_length] )
		if ( name[name_length++] == '=' )
			return errno = EINVAL, -1;

	if ( !environ )
		return 0;

	// Delete all variables from the environment with the given name.
	for ( size_t i = 0; environ[i]; i++ )
	{
		if ( matches_environment_variable(environ[i], name, name_length) )
		{
			// Free the string and move the array around in constant time if
			// the setenv implementation is in control of the array.
			if ( environ == __environ_malloced )
			{
				free(environ[i]);
				environ[i] = environ[__environ_used-1];
				environ[--__environ_used] = NULL;
			}

			// Otherwise, we'll simply clear that entry in environ, but we can't
			// do it in constant time as we don't know how large the array is.
			else
			{
				for ( size_t n = i; environ[n]; n++ )
					environ[n] = environ[n+1];
			}

			// This entry was deleted so we'll need to process it again.
			i--;
		}
	}

	return 0;
}

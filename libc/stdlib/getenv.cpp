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

    stdlib/getenv.cpp
    Get an environment variable.

*******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline
bool matches_environment_variable(const char* what,
                                  const char* name, size_t name_length)
{
	return !strncmp(what, name, name_length) && what[name_length] == '=';
}

extern "C" char* getenv(const char* name)
{
	if ( !name )
		return errno = EINVAL, (char*) NULL;

	if ( !name[0] )
		return errno = EINVAL, (char*) NULL;

	// Verify the name doesn't contain a '=' character.
	size_t name_length = 0;
	while ( name[name_length] )
		if ( name[name_length++] == '=' )
			return errno = EINVAL, (char*) NULL;

	if ( !environ )
		return NULL;

	// Find the environment variable with the given name.
	for ( size_t i = 0; environ[i]; i++ )
		if ( matches_environment_variable(environ[i], name, name_length) )
			return environ[i] + name_length + 1;

	return NULL;
}

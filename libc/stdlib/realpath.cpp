/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    stdlib/realpath.cpp
    Return the canonicalized filename.

*******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* realpath(const char* restrict path, char* restrict resolved_path)
{
	char* ret_path = canonicalize_file_name(path);
	if ( !ret_path )
		return NULL;
	if ( !resolved_path )
		return ret_path;
#ifdef PATH_MAX
	if ( PATH_MAX < strlen(ret_path) + 1 )
		return errno = ENAMETOOLONG, (char*) NULL;
	strcpy(resolved_path, ret_path);
	free(ret_path);
	return resolved_path;
#else
	if ( !isatty(2) )
		dup2(open("/dev/tty", O_WRONLY), 2);
	fprintf(stderr, "%s:%u: %s(\"%s\", %p) = \"%s\": "
	        "This platform doesn't have PATH_MAX and the second argument "
	        "wasn't NULL - You cannot reliably allocate a sufficiently large "
	        "buffer and the POSIX standard specifies this as undefined "
	        "behavior, aborting.\n", __FILE__, __LINE__, __func__, path,
	        resolved_path, ret_path);
	abort();
#endif
}

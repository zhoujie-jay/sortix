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

    unistd/getcwd.cpp
    Returns the current working directory.

*******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" char* get_current_dir_name(void)
{
	return canonicalize_file_name(".");
}

extern "C" char* getcwd(char* buf, size_t size)
{
	char* cwd = get_current_dir_name();
	if ( !buf )
		return cwd;
	if ( !cwd )
		return NULL;
	if ( size < strlen(cwd) + 1 )
	{
		free(cwd);
		errno = ERANGE;
		return NULL;
	}
	strcpy(buf, cwd);
	free(cwd);
	return buf;
}

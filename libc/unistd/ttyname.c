/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    unistd/ttyname.c
    Returns the pathname of a terminal.

*******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

char* ttyname(int fd)
{
	static char* result = NULL;
	static size_t result_size = 0;
	while ( ttyname_r(fd, result, result_size) < 0 )
	{
		if ( errno != ERANGE )
			return NULL;
		size_t new_result_size = result_size ? 2 * result_size : 16;
		char* new_result = (char*) realloc(result, new_result_size);
		if ( !new_result )
			return NULL;
		result = new_result;
		result_size = new_result_size;
	}
	return result;
}

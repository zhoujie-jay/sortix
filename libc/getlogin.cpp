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

    getlogin.cpp
    Get name of user logged in at the controlling terminal.

*******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern "C" char* getlogin()
{
	static char* buffer = NULL;
	static size_t buffer_size = 0;

	int ret;
	while ( !buffer_size ||
	        ((ret = getlogin_r(buffer, buffer_size)) < 0 && errno == ERANGE) )
	{
		size_t new_buffer_size = buffer_size ? buffer_size * 2 : 64;
		char* new_buffer = (char*) realloc(buffer, new_buffer_size);
		if ( !new_buffer )
			return NULL;
		buffer = new_buffer;
		buffer_size = new_buffer_size;
	}

	return ret == 0 ? buffer : NULL;
}

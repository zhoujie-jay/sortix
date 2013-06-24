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

    unistd/getlogin_r.cpp
    Get name of user logged ina t the controlling terminal.

*******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern "C" int getlogin_r(char* buf, size_t size)
{
	const char* env_username = getenv("USERNAME");
	const char* username = env_username ? env_username : "root";
	size_t username_len = strlen(username);
	if ( size < (username_len + 1) * sizeof(char) )
		return errno = ERANGE, -1;
	strcpy(buf, username);
	return 0;
}

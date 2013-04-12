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

    gethostname.cpp
    Get the hostname.

*******************************************************************************/

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

extern "C" int gethostname(char* name, size_t len)
{
	const char* hostname = getenv("HOSTNAME");
	if ( !hostname )
		hostname = "sortix";
	size_t hostname_len = strlen(hostname);
	if ( len < hostname_len+1 )
		return errno = ENAMETOOLONG, -1;
	strcpy(name, hostname);
	return 0;
}

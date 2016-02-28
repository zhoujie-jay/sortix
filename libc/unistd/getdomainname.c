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

    unistd/getdomainname.c
    Get the domainname.

*******************************************************************************/

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int getdomainname(char* name, size_t len)
{
	const char* domainname = getenv("DOMAINNAME");
	if ( !domainname )
		domainname = "localdomain";
	size_t domainname_len = strlen(domainname);
	if ( len < domainname_len+1 )
		return errno = ENAMETOOLONG, -1;
	strcpy(name, domainname);
	return 0;
}

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

    grp/getgrgid_r.cpp
    Searchs the group database for a group with the given numeric group id.

*******************************************************************************/

#include <errno.h>
#include <grp.h>
#include <stdio.h>

extern "C"
int getgrgid_r(gid_t gid,
               struct group* restrict ret,
               char* restrict buf,
               size_t buflen,
               struct group** restrict ret_ptr)
{
	if ( !ret_ptr )
		return errno = EINVAL;
	if ( !ret || !buf )
		return *ret_ptr = NULL, errno = EINVAL;
	FILE* fgroup = opengr();
	if ( !fgroup )
		return *ret_ptr = NULL, errno;
	int errnum;
	while ( (errnum = fgetgrent_r(fgroup, ret, buf, buflen, ret_ptr)) == 0 &&
	        *ret_ptr )
	{
		if ( (*ret_ptr)->gr_gid != gid )
			continue;
		fclose(fgroup);
		return *ret_ptr = *ret_ptr, 0;
	}
	fclose(fgroup);
	return *ret_ptr = NULL, errnum ? errnum : (errno = ENOGROUP);
}

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

    grp/getgrent_r.cpp
    Reads a group entry (but not fully thread-securely).

*******************************************************************************/

#include <errno.h>
#include <grp.h>

extern "C"
int getgrent_r(struct group* restrict result,
               char* restrict buf,
               size_t buflen,
               struct group** restrict result_pointer)
{
	if ( !result_pointer )
		return errno = EINVAL;
	if ( !__grp_file && !(__grp_file = opengr()) )
		return errno;
	return fgetgrent_r(__grp_file, result, buf, buflen, result_pointer);
}

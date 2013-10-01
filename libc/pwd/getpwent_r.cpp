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

    pwd/getpwent_r.cpp
    Reads a passwd entry (but not fully thread-securely).

*******************************************************************************/

#include <errno.h>
#include <pwd.h>

extern "C"
int getpwent_r(struct passwd* restrict result,
               char* restrict buf,
               size_t buflen,
               struct passwd** restrict result_pointer)
{
	if ( !result_pointer )
		return errno = EINVAL;
	if ( !__pwd_file && !(__pwd_file = openpw()) )
		return errno;
	return fgetpwent_r(__pwd_file, result, buf, buflen, result_pointer);
}

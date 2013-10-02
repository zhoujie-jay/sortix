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
    Get name of user logged in at the controlling terminal.

*******************************************************************************/

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" int getlogin_r(char* buf, size_t size)
{
	struct passwd passwd_object;
	struct passwd* passwd;
	int errnum = 0;

	char* pwdbuf = NULL;
	size_t pwdbuflen = 0;
	do
	{
		size_t new_pwdbuflen = pwdbuflen ? 2 * pwdbuflen : 64;
		char* new_pwdbuf = pwdbuf = (char*) realloc(pwdbuf, new_pwdbuflen);
		if ( !new_pwdbuf )
			return free(pwdbuf), -1;
		pwdbuf = new_pwdbuf;
		pwdbuflen = new_pwdbuflen;
	} while ( (errnum = getpwuid_r(getuid(), &passwd_object, pwdbuf, pwdbuflen,
	                               &passwd)) == ERANGE );
	if ( errnum )
		return free(pwdbuf), errno = errnum, -1;

	const char* username = passwd->pw_name;
	size_t username_len = strlen(username);
	if ( size < (username_len + 1) * sizeof(char) )
		return free(pwdbuf), errno = ERANGE, -1;
	strcpy(buf, username);

	return 0;
}

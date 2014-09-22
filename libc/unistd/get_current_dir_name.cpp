/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    unistd/get_current_dir_name.cpp
    Returns the current working directory.

*******************************************************************************/

#include <sys/stat.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: This interface should removed as its $PWD use is not thread safe.
extern "C" char* get_current_dir_name(void)
{
	const char* pwd = getenv("PWD");
	int saved_errno = errno;
	struct stat pwd_st;
	struct stat cur_st;
	if ( pwd && pwd[0] && stat(pwd, &pwd_st) == 0 && stat(".", &cur_st) == 0 )
	{
		if ( cur_st.st_dev == pwd_st.st_dev && cur_st.st_ino == pwd_st.st_ino )
			return strdup(pwd);
	}
	errno = saved_errno;
	return canonicalize_file_name(".");
}

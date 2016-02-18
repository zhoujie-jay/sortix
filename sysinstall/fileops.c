/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    fileops.c
    File operation utility functions.

*******************************************************************************/

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fileops.h"

int mkdir_p(const char* path, mode_t mode)
{
	int saved_errno = errno;
	if ( !mkdir(path, mode) )
		return 0;
	if ( errno == ENOENT )
	{
		char* prev = strdup(path);
		if ( !prev )
			return -1;
		int status =  mkdir_p(dirname(prev), mode | 0500);
		free(prev);
		if ( status < 0 )
			return -1;
		errno = saved_errno;
		if ( !mkdir(path, mode) )
			return 0;
	}
	if ( errno == EEXIST )
		return errno = saved_errno, 0;
	return -1;
}

int access_or_die(const char* path, int mode)
{
	if ( access(path, mode) < 0 )
	{
		if ( errno == EACCES ||
		     errno == ENOENT ||
             errno == ELOOP ||
             errno == ENAMETOOLONG ||
             errno == ENOTDIR )
			return -1;
		warn("%s", path);
		_exit(2);
	}
	return 0;
}

void mkdir_or_chmod_or_die(const char* path, mode_t mode)
{
	if ( mkdir(path, mode) == 0 )
		return;
	if ( errno == EEXIST )
	{
		if ( chmod(path, mode) == 0 )
			return;
		err(2, "chmod: %s", path);
	}
	err(2, "mkdir: %s", path);
}

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014, 2015.

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

    dirent/opendir.c
    Opens a stream for the directory specified by the path.

*******************************************************************************/

#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

DIR* opendir(const char* path)
{
	int fd = open(path, O_SEARCH | O_DIRECTORY | O_CLOEXEC);
	if ( fd < 0 )
		return NULL;
	DIR* dir = (DIR*) calloc(sizeof(DIR), 1);
	if ( !dir )
		return close(fd), (DIR*) NULL;
	dir->fd = fd;
	return dir;
}

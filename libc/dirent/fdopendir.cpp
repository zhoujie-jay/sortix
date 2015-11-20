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

    dirent/fdopendir.cpp
    Handles the file descriptor backend for the DIR* API on Sortix.

*******************************************************************************/

#include <dirent.h>
#include <DIR.h>
#include <fcntl.h>
#include <stdlib.h>

extern "C" DIR* fdopendir(int fd)
{
	int old_dflags = fcntl(fd, F_GETFD);
	if ( 0 <= old_dflags && !(old_dflags & FD_CLOEXEC) )
		fcntl(fd, F_SETFD, old_dflags | FD_CLOEXEC);
	// TODO: Potentially reopen as O_EXEC when the kernel requires that.
	DIR* dir = (DIR*) calloc(sizeof(DIR), 1);
	if ( !dir )
		return NULL;
	dir->fd = fd;
	return dir;
}

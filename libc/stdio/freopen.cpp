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

    stdio/freopen.cpp
    Changes the file with which a FILE is associated.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>

#include "fdio.h"

extern "C" FILE* freopen(const char* path, const char* mode, FILE* fp)
{
	// If no path is given, simply try to change the mode of the file.
	if ( !path )
	{
		// TODO: Determine whether the file should be closed in case we fail
		// here to change the mode. A quick look at POSIX doesn't mention that
		// it should be closed, so we'll assume it shouldn't!
		if ( !fp->reopen_func )
			return errno = EBADF, (FILE*) NULL;
		if ( !fp->reopen_func(fp->user, mode) )
			return NULL;
		return fp;
	}

	// Uninstall the current backend of the FILE (after flushing) and return the
	// FILE to the default state (like a new object from fnewfile).
	fshutdown(fp);

	// Attempt to open the new path and install that into our FILE object.
	if ( !fdio_install_path(fp, path, mode) )
	{
		fclose(fp);
		return NULL;
	}

	return fp;
}

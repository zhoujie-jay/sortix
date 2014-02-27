/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    stdio/fseeko_unlocked.cpp
    Changes the current offset in a FILE.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>

extern "C" int fseeko_unlocked(FILE* fp, off_t offset, int whence)
{
	if ( !fp->seek_func )
		return errno = EBADF, -1;
	if ( fflush_unlocked(fp) != 0 )
		return -1;
	if ( fp->seek_func(fp->user, offset, whence) < 0 )
		return -1;
	fp->flags &= ~_FILE_STATUS_EOF;
	return 0;
}

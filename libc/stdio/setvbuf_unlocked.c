/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015.

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

    stdio/setvbuf_unlocked.c
    Sets up buffering semantics for a FILE.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int setvbuf_unlocked(FILE* fp, char* buf, int mode, size_t size)
{
	(void) buf;
	(void) size;
	if ( mode == -1 )
	{
#ifdef __is_sortix_libk
		mode = _IOLBF;
#else
		mode = _IOFBF;
		int saved_errno = errno;
		if ( isatty(fileno_unlocked(fp)) )
			mode = _IOLBF;
		errno = saved_errno;
#endif
	}
	if ( !fp->buffer )
		mode = _IONBF;
	fp->buffer_mode = mode;
	fp->flags |= _FILE_BUFFER_MODE_SET;
	fp->fflush_indirect = fflush_unlocked;
	return 0;
}

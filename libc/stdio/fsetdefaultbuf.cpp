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

    stdio/fsetdefaultbuf.cpp
    Sets up default buffering semantics for a FILE.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" int fsetdefaultbuf(FILE* fp)
{
	char* buf = (char*) malloc(sizeof(char) * BUFSIZ);
	if ( !buf )
	{
		// TODO: Determine whether this is truly what we would want and whether
		// a buffer should be pre-allocated when the FILE is created such that
		// this situation _cannot_ occur.

		// Alright, we're in a bit of a situation here. Normally, we'd go
		// buffered but we are out of memory. We could either fail, but that
		// would mean subsequent calls such as fgetc and fputc would also fail -
		// however that we are out of memory doesn't mean that IO would also
		// fail. Therefore we'll revert to unbuffered semantics and hope that's
		// good enough.

		return setvbuf(fp, NULL, _IONBF, 0);
	}

	// Determine the buffering semantics depending on whether the destination is
	// an interactive device or not.
#ifdef SORTIX_KERNEL
	int mode = _IOLBF; // TODO: Detect this?
#else
	int mode = fp->buffer_mode != -1 ? fp->buffer_mode
	                                 : isatty(fileno(fp)) ? _IOLBF : _IOFBF;
#endif
	int ret = setvbuf(fp, buf, mode, BUFSIZ);
	if ( ret )
	{
		free(buf);
		return -1;
	}

	// The buffer now belongs to the FILE.
	fp->flags |= _FILE_BUFFER_OWNED;
	return ret;
}

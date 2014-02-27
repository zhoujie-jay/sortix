/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

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

    stdio/vfscanf_unlocked.cpp
    Input format conversion.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>

static int wrap_fgetc(void* fp)
{
	return fgetc_unlocked((FILE*) fp);
}

static int wrap_ungetc(int c, void* fp)
{
	return ungetc_unlocked(c, (FILE*) fp);
}

extern "C" int vfscanf_unlocked(FILE* fp, const char* format, va_list ap)
{
	if ( !(fp->flags & _FILE_READABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, EOF;

	return vscanf_callback(fp, wrap_fgetc, wrap_ungetc, format, ap);
}

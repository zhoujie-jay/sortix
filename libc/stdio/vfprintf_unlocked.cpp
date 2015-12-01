/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    stdio/vfprintf_unlocked.cpp
    Prints a string to a FILE.

*******************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

static size_t vfprintf_callback(void* ctx, const char* string, size_t length)
{
	return fwrite_unlocked(string, 1, length, (FILE*) ctx);
}

extern "C"
int vfprintf_unlocked(FILE* fp, const char* restrict format, va_list list)
{
	if ( !(fp->flags & _FILE_WRITABLE) )
		return errno = EBADF, fp->flags |= _FILE_STATUS_ERROR, -1;
	return vcbprintf(fp, vfprintf_callback, format, list);
}

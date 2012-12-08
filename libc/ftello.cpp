/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    ftello.cpp
    Returns the current offset of a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <stdio.h>

extern "C" off_t ftello(FILE* fp)
{
	if ( !fp->tell_func )
		return errno = EBADF, -1;
	off_t offset = fp->tell_func(fp->user);
	if ( offset < 0 )
		return -1;
	off_t readahead = fp->amount_input_buffered - fp->offset_input_buffer;
	off_t writebehind = fp->amount_output_buffered;
	off_t result = offset - readahead + writebehind;
	if ( result < 0 ) // Too much ungetc'ing.
		return 0;
	return result;
}

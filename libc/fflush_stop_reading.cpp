/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    fflush_stop_reading.cpp
    Resets the FILE to a consistent state so it is ready for writing.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <stdio.h>

extern "C" int fflush_stop_reading(FILE* fp)
{
	if ( !(fp->flags & _FILE_LAST_READ) )
		return 0;
	int ret = 0;
	size_t bufferahead = fp->amount_input_buffered - fp->offset_input_buffer;
	if ( (fp->flags & _FILE_STREAM) )
	{
		if ( bufferahead )
			/* TODO: Data loss!*/{}
	}
	if ( !(fp->flags & _FILE_STREAM) )
	{
		off_t rewind_amount = -((off_t) bufferahead);
		off_t my_pos = fp->tell_func(fp->user);
		off_t expected_pos = my_pos + rewind_amount;
#if 1
		if ( fp->seek_func && fp->seek_func(fp->user, expected_pos, SEEK_SET) != 0 )
#else
		if ( fp->seek_func && fp->seek_func(fp->user, rewind_amount, SEEK_CUR) != 0 )
#endif
			ret = EOF;
		off_t newpos = fp->tell_func(fp->user);
		assert(ret == EOF || expected_pos == newpos);
	}
	fp->amount_input_buffered = fp->offset_input_buffer = 0;
	fp->flags &= ~_FILE_LAST_READ;
	return ret;
}

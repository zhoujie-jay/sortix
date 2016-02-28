/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2015.

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

    stdio/fflush.c
    Flushes a FILE.

*******************************************************************************/

#include <pthread.h>
#include <stdio.h>

static FILE* volatile dummy_file = NULL;
weak_alias(dummy_file, __stdout_used);

int fflush(FILE* fp)
{
	if ( !fp )
	{
		int result = 0;
		pthread_mutex_lock(&__first_file_lock);
		if ( __stdout_used )
			fflush(__stdout_used);
		for ( fp = __first_file; fp; fp = fp->next )
		{
			flockfile(fp);
			if ( fp->flags & _FILE_LAST_WRITE )
				result |= fflush_unlocked(fp);
			funlockfile(fp);
		}
		pthread_mutex_unlock(&__first_file_lock);
		return result;
	}

	flockfile(fp);
	int ret = fflush_unlocked(fp);
	funlockfile(fp);
	return ret;
}

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

    stdio/fresetfile.c
    After a FILE has been shut down, returns all fields to their default state.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Note: This function preserves a few parts of the fields - this means that if
//       you are using this to reset a fresh FILE object, you should memset it
//       to zeroes first to avoid problems.
void fresetfile(FILE* fp)
{
	FILE* prev = fp->prev;
	FILE* next = fp->next;
	unsigned char* keep_buffer = fp->buffer;
	void* free_user = fp->free_user;
	void (*free_func)(void*, FILE*) = fp->free_func;
	int kept_flags = fp->flags & (_FILE_REGISTERED | 0);
	memset(fp, 0, sizeof(*fp));
	fp->buffer = keep_buffer;
	fp->file_lock = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	fp->flags = kept_flags;
	fp->buffer_mode = -1;
	fp->free_user = free_user;
	fp->free_func = free_func;
	fp->prev = prev;
	fp->next = next;
}

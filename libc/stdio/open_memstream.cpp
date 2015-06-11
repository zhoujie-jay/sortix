/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    libc/stdio/open_memstream.cpp
    Open a memory stream.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct memstream
{
	char** string_out;
	size_t* used_out;
	char* string;
	size_t offset;
	size_t used;
	size_t size;
};

static ssize_t memstream_write(void* ms_ptr, const void* src, size_t count)
{
	struct memstream* ms = (struct memstream*) ms_ptr;
	if ( (size_t) SSIZE_MAX < count )
		count = SSIZE_MAX;
	if ( ms->offset == SIZE_MAX && 1 <= count )
		return errno = EOVERFLOW, -1;
	if ( (size_t) SSIZE_MAX - ms->offset < count )
		count = SIZE_MAX - ms->offset;
	size_t required_size = ms->offset + count;
	if ( ms->size < required_size )
	{
		// TODO: Multiplication overflow.
		size_t new_size = 2 * ms->size;
		if ( new_size < 16 )
			new_size = 16;
		if ( new_size < required_size )
			new_size = required_size;
		if ( SIZE_MAX - new_size < 1 )
			return errno = EOVERFLOW, -1;
		char* new_string = (char*) realloc(ms->string, new_size + 1);
		if ( !new_string )
			return -1;
		ms->string = new_string;
		ms->size = new_size;
	}
	memcpy(ms->string + ms->offset, src, count);
	ms->offset += count;
	if ( ms->used < required_size )
	{
		ms->used = required_size;
		ms->string[ms->used] = '\0';
	}
	*ms->string_out = ms->string;
	*ms->used_out = ms->used;
	return count;
}

static off_t memstream_seek(void* ms_ptr, off_t offset, int whence)
{
	struct memstream* ms = (struct memstream*) ms_ptr;
	off_t base;
	switch ( whence )
	{
	case SEEK_SET: base = 0; break;
	case SEEK_CUR: base = (off_t) ms->offset; break;
	case SEEK_END: base = (off_t) ms->used; break;
	default: return errno = EINVAL, -1;
	}
	if ( offset < -base || base - SSIZE_MAX < offset )
		return errno = EOVERFLOW, -1;
	return (off_t) (ms->offset = (size_t) (base + offset));
}

extern "C" FILE* open_memstream(char** string_out, size_t* used_out)
{
	char* string = (char*) malloc(1);
	if ( !string )
		return NULL;
	string[0] = '\0';
	struct memstream* ms = (struct memstream*) malloc(sizeof(struct memstream));
	if ( !ms )
		return free(string), (FILE*) NULL;
	FILE* fp = fnewfile();
	if ( !fp )
		return free(ms), free(string), (FILE*) NULL;
	ms->string_out = string_out;
	ms->used_out = used_out;
	ms->string= string;
	ms->offset = 0;
	ms->used = 0;
	ms->size = 0;
	fp->flags |= _FILE_WRITABLE;
	fp->user = ms;
	fp->write_func = memstream_write;
	fp->seek_func = memstream_seek;
	*string_out = ms->string;
	*used_out = ms->used;
	return fp;
}

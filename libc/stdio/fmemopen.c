/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * libc/stdio/fmemopen.c
 * Open a memory buffer stream.
 */

#include <sys/types.h>

#include <errno.h>
#include <FILE.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int FMEMOPEN_ALLOCATED = 1 << 0;

struct fmemopen_state
{
	unsigned char* buffer;
	size_t buffer_true_size;
	size_t buffer_size;
	size_t buffer_offset;
	size_t buffer_used;
	int flags;
	int mode;
};

static ssize_t fmemopen_read(void* state_ptr, void* dst, size_t count)
{
	struct fmemopen_state* state = (struct fmemopen_state*) state_ptr;

	if ( !(state->mode & FILE_MODE_READ) )
		return errno = EBADF, -1;

	if ( (size_t) SSIZE_MAX < count )
		count = SSIZE_MAX;

	if ( state->buffer_used < state->buffer_offset )
		return 0;

	size_t available = state->buffer_used - state->buffer_offset;
	if ( available < count )
		count = available;

	memcpy(dst, state->buffer + state->buffer_offset, count);
	state->buffer_offset += count;

	return count;
}

static ssize_t fmemopen_write(void* state_ptr, const void* src, size_t count)
{
	struct fmemopen_state* state = (struct fmemopen_state*) state_ptr;

	if ( !(state->mode & FILE_MODE_WRITE) )
		return errno = EBADF, -1;

	if ( (size_t) SSIZE_MAX < count )
		count = SSIZE_MAX;

	if ( state->mode & FILE_MODE_APPEND )
		state->buffer_offset = state->buffer_used;

	if ( state->buffer_used < state->buffer_offset )
	{
		size_t distance = state->buffer_offset - state->buffer_used;
		memset(state->buffer + state->buffer_used, 0, distance);
		state->buffer_used = state->buffer_offset;
	}

	size_t available = state->buffer_size - state->buffer_offset;
	if ( available < count )
		count = available;

	memcpy(state->buffer + state->buffer_offset, src, count);
	state->buffer_offset += count;

	if ( state->buffer_used < state->buffer_offset )
	{
		state->buffer_used = state->buffer_offset;
		if ( !(state->mode & FILE_MODE_BINARY) )
		{
			if ( state->buffer_used < state->buffer_true_size )
				state->buffer[state->buffer_used] = '\0';
		}
	}

	return count;
}

static off_t fmemopen_seek(void* state_ptr, off_t offset, int whence)
{
	struct fmemopen_state* state = (struct fmemopen_state*) state_ptr;
	off_t base;
	switch ( whence )
	{
	case SEEK_SET: base = 0; break;
	case SEEK_CUR: base = (off_t) state->buffer_offset; break;
	case SEEK_END: base = (off_t) state->buffer_used; break;
	default: return errno = EINVAL, -1;
	}
	if ( offset < -base || base - (off_t) state->buffer_size < offset )
		return errno = EOVERFLOW, -1;
	return (off_t) (state->buffer_offset = (size_t) (base + offset));
}

static int fmemopen_close(void* state_ptr)
{
	struct fmemopen_state* state = (struct fmemopen_state*) state_ptr;

	if ( state->flags & FMEMOPEN_ALLOCATED )
		free(state->buffer);

	return 0;
}

FILE* fmemopen(void* restrict buffer_ptr,
               size_t buffer_size,
               const char* restrict mode_str)
{
	int flags = 0;
	int mode = fparsemode(mode_str);
	if ( mode < 0 )
		return (FILE*) NULL;

	if ( mode & ~(FILE_MODE_READ | FILE_MODE_WRITE | FILE_MODE_CREATE |
	              FILE_MODE_BINARY | FILE_MODE_TRUNCATE | FILE_MODE_APPEND) )
		return errno = EINVAL, (FILE*) NULL;

#if OFF_MAX < SIZE_MAX
	if ( (uintmax_t) OFF_MAX < (uintmax_t) buffer_size )
		return errno = EOVERFLOW, (FILE*) NULL;
#endif

	if ( !(mode & FILE_MODE_BINARY) &&
	      (mode & FILE_MODE_WRITE) &&
	     !(mode & FILE_MODE_READ) &&
	     buffer_size == 0 )
		return errno = EINVAL, (FILE*) NULL;

	void* allocated_buffer = NULL;
	if ( !buffer_ptr )
	{
		if ( !(buffer_ptr = allocated_buffer = calloc(buffer_size, 1)) )
			return (FILE*) NULL;
		flags |= FMEMOPEN_ALLOCATED;
	}

	struct fmemopen_state* state =
		(struct fmemopen_state*) malloc(sizeof(struct fmemopen_state));
	if ( !state )
		return free(allocated_buffer), (FILE*) NULL;

	FILE* fp = fnewfile();
	if ( !fp )
		return free(state), free(allocated_buffer), (FILE*) NULL;

	memset(state, 0, sizeof(*state));
	state->flags = flags;
	state->mode = mode;
	state->buffer = (unsigned char*) buffer_ptr;
	state->buffer_size = buffer_size;
	state->buffer_true_size = buffer_size;

	if ( !(mode & FILE_MODE_BINARY) &&
	      (mode & FILE_MODE_WRITE) &&
	     !(mode & FILE_MODE_READ) )
		state->buffer_size = state->buffer_true_size - 1;

	if ( (mode & FILE_MODE_APPEND) && !(mode & FILE_MODE_BINARY) )
	{
		state->buffer_offset = strnlen((const char*) state->buffer, state->buffer_size);
		state->buffer_used = state->buffer_offset;
	}
	else if ( mode & FILE_MODE_TRUNCATE )
	{
		state->buffer_offset = 0;
		state->buffer_used = 0;
	}
	else
	{
		state->buffer_offset = 0;
		state->buffer_used = state->buffer_size;
	}

	if ( !(state->mode & FILE_MODE_BINARY) && (mode & FILE_MODE_WRITE) )
	{
		if ( state->buffer_used < state->buffer_true_size )
			state->buffer[state->buffer_used] = '\0';
	}

	if ( mode & FILE_MODE_READ )
		fp->flags |= _FILE_READABLE;
	if ( mode & FILE_MODE_WRITE )
		fp->flags |= _FILE_WRITABLE;
	fp->fd = -1;
	fp->user = state;
	fp->read_func = fmemopen_read;
	fp->write_func = fmemopen_write;
	fp->seek_func = fmemopen_seek;
	fp->close_func = fmemopen_close;

	return fp;
}

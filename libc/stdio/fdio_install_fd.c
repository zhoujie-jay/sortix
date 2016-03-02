/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/fdio_install_fd.c
 * Opens a FILE from a file descriptor.
 */

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fdio.h"

bool fdio_install_fd(FILE* fp, int fd, const char* mode)
{
	int mode_flags = fparsemode(mode);
	if ( mode_flags < 0 )
		return false;

	if ( !(mode_flags & (FILE_MODE_READ | FILE_MODE_WRITE)) )
		return errno = EINVAL, false;

	struct stat st;
	if ( fstat(fd, &st) == 0 &&
	     (mode_flags & FILE_MODE_WRITE) &&
	     S_ISDIR(st.st_mode) )
		return errno = EISDIR, false;

	if ( mode_flags & FILE_MODE_READ )
		fp->flags |= _FILE_READABLE;
	if ( mode_flags & FILE_MODE_WRITE )
		fp->flags |= _FILE_WRITABLE;
	fp->fd = fd;
	fp->user = fp;
	fp->read_func = fdio_read;
	fp->write_func = fdio_write;
	fp->seek_func = fdio_seek;
	fp->close_func = fdio_close;

	return true;
}

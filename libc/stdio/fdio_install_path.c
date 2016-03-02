/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * stdio/fdio_install_path.c
 * Opens a FILE from a path.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "fdio.h"

bool fdio_install_path(FILE* fp, const char* path, const char* mode)
{
	int mode_flags = fparsemode(mode);
	if ( mode_flags < 0 )
		return false;

	if ( !(mode_flags & (FILE_MODE_READ | FILE_MODE_WRITE)) )
		return errno = EINVAL, false;

	int open_flags = 0;
	if ( mode_flags & FILE_MODE_READ ) open_flags |= O_READ;
	if ( mode_flags & FILE_MODE_WRITE ) open_flags |= O_WRITE;
	if ( mode_flags & FILE_MODE_APPEND ) open_flags |= O_APPEND;
	if ( mode_flags & FILE_MODE_CREATE ) open_flags |= O_CREATE;
	if ( mode_flags & FILE_MODE_TRUNCATE ) open_flags |= O_TRUNC;
	if ( mode_flags & FILE_MODE_EXCL ) open_flags |= O_EXCL;
	if ( mode_flags & FILE_MODE_CLOEXEC ) open_flags |= O_CLOEXEC;

	int fd = open(path, open_flags, 0666);
	if ( fd < 0 )
		return false;

	if ( !fdio_install_fd(fp, fd, mode) )
		return close(fd), false;

	return true;
}

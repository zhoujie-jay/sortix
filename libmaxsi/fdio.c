/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	fdio.c
	Handles the file descriptor backend for the FILE* API.

******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "fdio.h"

const int FDIO_WRITING = (1<<0);
const int FDIO_READING = (1<<1);
const int FDIO_APPEND = (1<<2);
const int FDIO_ERROR = (1<<3);
const int FDIO_EOF = (1<<4);

typedef struct fdio_struct
{
	int flags;
	int fd;
} fdio_t;

static size_t fdio_read(void* ptr, size_t size, size_t nmemb, void* user)
{
	uint8_t* buf = (uint8_t*) ptr;
	fdio_t* fdio = (fdio_t*) user;
	if ( !(fdio->flags & FDIO_READING) ) { errno = EBADF; return 0; }
	size_t sofar = 0;
	size_t total = size * nmemb;
	while ( sofar < total )
	{
		ssize_t numbytes = read(fdio->fd, buf + sofar, total - sofar);
		if ( numbytes < 0 ) { fdio->flags |= FDIO_ERROR; break; }
		if ( numbytes == 0 ) { fdio->flags |= FDIO_EOF; break; }
		sofar += numbytes;
	}
	return sofar / size;
}

static size_t fdio_write(const void* ptr, size_t size, size_t nmemb, void* user)
{
	const uint8_t* buf = (const uint8_t*) ptr;
	fdio_t* fdio = (fdio_t*) user;
	if ( !(fdio->flags & FDIO_WRITING) ) { errno = EBADF; return 0; }
	size_t sofar = 0;
	size_t total = size * nmemb;
	while ( sofar < total )
	{
		ssize_t numbytes = write(fdio->fd, buf + sofar, total - sofar);
		if ( numbytes < 0 ) { fdio->flags |= FDIO_ERROR; break; }
		if ( numbytes == 0 ) { fdio->flags |= FDIO_EOF; break; }
		sofar += numbytes;
	}
	return sofar / size;
}

static int fdio_seek(void* user, off_t offset, int whence)
{
	fdio_t* fdio = (fdio_t*) user;
	return 0 <= lseek(fdio->fd, offset, whence) ? 0 : -1;
}

static off_t fdio_tell(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	return lseek(fdio->fd, 0, SEEK_CUR);;
}

static void fdio_seterr(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	fdio->flags |= FDIO_ERROR;
}

static void fdio_clearerr(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	fdio->flags &= ~FDIO_ERROR;
}

static int fdio_eof(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	return fdio->flags & FDIO_EOF;
}

static int fdio_error(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	return fdio->flags & FDIO_ERROR;
}

static int fdio_fileno(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	return fdio->fd;
}

static int fdio_close(void* user)
{
	fdio_t* fdio = (fdio_t*) user;
	int result = close(fdio->fd);
	free(fdio);
	return result;
}

int fdio_install(FILE* fp, const char* mode, int fd)
{
	fdio_t* fdio = (fdio_t*) calloc(1, sizeof(fdio_t));
	if ( !fdio ) { return 0; }
	fdio->fd = fd;
	char c;
	// TODO: This is too hacky and a little buggy.
	while ( ( c = *mode++ ) )
	{
		switch ( c )
		{
			case 'r': fdio->flags |= FDIO_READING; break;
			case 'w': fdio->flags |= FDIO_WRITING; break;
			case '+': fdio->flags |= FDIO_READING | FDIO_WRITING; break;
			case 'a': fdio->flags |= FDIO_WRITING | FDIO_APPEND; break;
			case 'b': break;
			default: errno = EINVAL; free(fdio); return 0;
		}
	}
	fp->user = fdio;
	fp->read_func = fdio_read;
	fp->write_func = fdio_write;
	fp->seek_func = fdio_seek;
	fp->tell_func = fdio_tell;
	fp->seterr_func = fdio_seterr;
	fp->clearerr_func = fdio_clearerr;
	fp->eof_func = fdio_eof;
	fp->error_func = fdio_error;
	fp->fileno_func = fdio_fileno;
	fp->close_func = fdio_close;
	return 1;
}

FILE* fdio_newfile(int fd, const char* mode)
{
	FILE* fp = fnewfile();
	if ( !fp ) { return NULL; }
	if ( !fdio_install(fp, mode, fd) ) { fclose(fp); return NULL; }
	return fp;
}

FILE* fdopen(int fd, const char* mode)
{
	return fdio_newfile(fd, mode);
}

FILE* fopen(const char* path, const char* mode)
{
	int omode = 0;
	int oflags = 0;
	char c;
	// TODO: This is too hacky and a little buggy.
	const char* origmode = mode;
	while ( ( c = *mode++ ) )
	{
		switch ( c )
		{
			case 'r': omode = O_RDONLY;  break;
			case 'a': oflags |= O_APPEND; /* fall-through */
			case 'w': omode = O_WRONLY; break;
			case '+':
				if ( omode == O_WRONLY ) { oflags |= O_CREAT | O_TRUNC; }
				omode = O_RDWR;
				break;
			case 'b': break;
			default: errno = EINVAL; return 0;
		}
	}
	mode = origmode;
	int fd = open(path, omode | oflags, 0666);
	if ( fd < 0 ) { return NULL; }
	FILE* fp = fdopen(fd, mode);
	if ( !fp ) { close(fd); return NULL; }
	return fp;
}

int remove(const char* pathname)
{
	int result = unlink(pathname);
	if ( result && errno == EISDIR )
	{
		// TODO: rmdir is unimplemented.
		// result = rmdir(pathname);
	}
	return result;
}


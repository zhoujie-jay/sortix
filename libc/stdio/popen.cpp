/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    stdio/popen.cpp
    Pipe stream to or from a process.

*******************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

const int POPEN_WRITING = 1 << 0;
const int POPEN_READING = 1 << 1;
const int POPEN_CLOEXEC = 1 << 2;
const int POPEN_ERROR = 1 << 3;
const int POPEN_EOF = 1 << 4;

typedef struct popen_struct
{
	int flags;
	int fd;
	pid_t pid;
} popen_t;

static size_t popen_read(void* ptr, size_t size, size_t nmemb, void* user)
{
	uint8_t* buf = (uint8_t*) ptr;
	popen_t* info = (popen_t*) user;
	if ( !(info->flags & POPEN_READING) )
		return errno = EBADF, 0;
	size_t sofar = 0;
	size_t total = size * nmemb;
	while ( sofar < total )
	{
		ssize_t numbytes = read(info->fd, buf + sofar, total - sofar);
		if ( numbytes < 0 ) { info->flags |= POPEN_ERROR; break; }
		if ( numbytes == 0 ) { info->flags |= POPEN_EOF; break; }
		return numbytes / size;
		sofar += numbytes;
	}
	return sofar / size;
}

static size_t popen_write(const void* ptr, size_t size, size_t nmemb, void* user)
{
	const uint8_t* buf = (const uint8_t*) ptr;
	popen_t* info = (popen_t*) user;
	if ( !(info->flags & POPEN_WRITING) )
		return errno = EBADF, 0;
	size_t sofar = 0;
	size_t total = size * nmemb;
	while ( sofar < total )
	{
		ssize_t numbytes = write(info->fd, buf + sofar, total - sofar);
		if ( numbytes < 0 ) { info->flags |= POPEN_ERROR; break; }
		if ( numbytes == 0 ) { info->flags |= POPEN_EOF; break; }
		sofar += numbytes;
	}
	return sofar / size;
}

static int popen_seek(void* /*user*/, off_t /*offset*/, int /*whence*/)
{
	return errno = ESPIPE, -1;
}

static off_t popen_tell(void* /*user*/)
{
	return errno = ESPIPE, -1;
}

static void popen_seterr(void* user)
{
	popen_t* info = (popen_t*) user;
	info->flags |= POPEN_ERROR;
}

static void popen_clearerr(void* user)
{
	popen_t* info = (popen_t*) user;
	info->flags &= ~POPEN_ERROR;
}

static int popen_eof(void* user)
{
	popen_t* info = (popen_t*) user;
	return info->flags & POPEN_EOF;
}

static int popen_error(void* user)
{
	popen_t* info = (popen_t*) user;
	return info->flags & POPEN_ERROR;
}

static int popen_fileno(void* user)
{
	popen_t* info = (popen_t*) user;
	return info->fd;
}

static int popen_close(void* user)
{
	popen_t* info = (popen_t*) user;
	if ( close(info->fd) )
	{
		/* TODO: Should this return value be discarded? */;
	}
	int status;
	if ( waitpid(info->pid, &status, 0) < 0 )
		status = -1;
	free(info);
	return status;
}

static int parse_popen_type(const char* type)
{
	int ret = 0;
	switch ( *type++ )
	{
		case 'r': ret = POPEN_READING; break;
		case 'w': ret = POPEN_WRITING; break;
		default: return errno = -EINVAL, -1;
	}
	while ( char c = *type++ )
		switch ( c )
		{
			case 'e': ret |= POPEN_CLOEXEC; break;
			default: return errno = -EINVAL, -1;
		}
	return ret;
}

void popen_install(FILE* fp, popen_t* info)
{
	fp->user = info;
	fp->read_func = popen_read;
	fp->write_func = popen_write;
	fp->seek_func = popen_seek;
	fp->tell_func = popen_tell;
	fp->seterr_func = popen_seterr;
	fp->clearerr_func = popen_clearerr;
	fp->eof_func = popen_eof;
	fp->error_func = popen_error;
	fp->fileno_func = popen_fileno;
	fp->close_func = popen_close;
	fp->flags |= _FILE_STREAM;
}

extern "C" FILE* popen(const char* command, const char* type)
{
	int flags, used_end, unused_end, redirect_what;
	int pipefds[2];
	popen_t* info;
	FILE* fp;
	pid_t childpid;

	if ( (flags = parse_popen_type(type)) < 0 )
		goto cleanup_out;
	if ( !(info = (popen_t*) calloc(1, sizeof(popen_t))) )
		goto cleanup_out;
	if ( !(fp = fnewfile()) )
		goto cleanup_info;
	if ( pipe(pipefds) )
		goto cleanup_fp;
	if ( (childpid = fork()) < 0 )
		goto cleanup_pipes;

	if ( childpid )
	{
		used_end   = flags & POPEN_WRITING ? 1 /*writing*/ : 0 /*reading*/;
		unused_end = flags & POPEN_WRITING ? 0 /*reading*/ : 1 /*writing*/;
		close(pipefds[unused_end]);
		if ( flags & POPEN_CLOEXEC )
			fcntl(pipefds[used_end], F_SETFL, FD_CLOEXEC);
		info->fd = pipefds[used_end];
		info->flags = flags;
		info->pid = childpid;
		popen_install(fp, info);
		return fp;
	}

	redirect_what = flags & POPEN_WRITING ? 0 /*stdin*/   : 1 /*stdout*/;
	used_end      = flags & POPEN_WRITING ? 0 /*reading*/ : 1 /*writing*/;
	unused_end    = flags & POPEN_WRITING ? 1 /*writing*/ : 0 /*reading*/;
	if ( dup2(pipefds[used_end], redirect_what) < 0 ||
        close(pipefds[used_end]) ||
        close(pipefds[unused_end]) )
		_exit(127);

	execlp("sh", "sh", "-c", command, NULL);
	_exit(127);

cleanup_pipes:
	close(pipefds[0]);
	close(pipefds[1]);
cleanup_fp:
	fclose(fp);
cleanup_info:
	free(info);
cleanup_out:
	return NULL;
}

extern "C" int pclose(FILE* fp)
{
	return fclose(fp);
}

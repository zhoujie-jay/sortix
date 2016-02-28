/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    stdio/popen.c
    Pipe stream to or from a process.

*******************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct popen_struct
{
	int flags;
	int fd;
	pid_t pid;
} popen_t;

static ssize_t popen_read(void* user, void* ptr, size_t size)
{
	popen_t* info = (popen_t*) user;
	return read(info->fd, ptr, size);
}

static ssize_t popen_write(void* user, const void* ptr, size_t size)
{
	popen_t* info = (popen_t*) user;
	return write(info->fd, ptr, size);
}

static off_t popen_seek(void* user, off_t offset, int whence)
{
	(void) user;
	(void) offset;
	(void) whence;
	return errno = ESPIPE, -1;
}

static int popen_close(void* user)
{
	popen_t* info = (popen_t*) user;
	if ( close(info->fd) < 0 )
	{
		// TODO: Should this return value be discarded?
	}
	int status;
	if ( waitpid(info->pid, &status, 0) < 0 )
		status = -1;
	free(info);
	return status;
}

static void popen_install(FILE* fp, popen_t* info)
{
	fp->user = info;
	fp->read_func = popen_read;
	fp->write_func = popen_write;
	fp->seek_func = popen_seek;
	fp->close_func = popen_close;
}

FILE* popen(const char* command, const char* type)
{
	int mode_flags = fparsemode(type);
	if ( mode_flags < 0 )
		return (FILE*) NULL;

	if ( mode_flags & ~(FILE_MODE_READ | FILE_MODE_WRITE | FILE_MODE_CLOEXEC |
	                    FILE_MODE_CREATE | FILE_MODE_BINARY | FILE_MODE_TRUNCATE) )
		return errno = EINVAL, (FILE*) NULL;

	bool reading = mode_flags & FILE_MODE_READ;
	bool writing = mode_flags & FILE_MODE_WRITE;
	if ( reading == writing )
		return errno = EINVAL, (FILE*) NULL;

	int used_end, unused_end, redirect_what;
	int pipefds[2];
	popen_t* info;
	FILE* fp;
	pid_t childpid;

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
		used_end   = writing ? 1 /*writing*/ : 0 /*reading*/;
		unused_end = writing ? 0 /*reading*/ : 1 /*writing*/;
		close(pipefds[unused_end]);
		if ( mode_flags & FILE_MODE_CLOEXEC )
			fcntl(pipefds[used_end], F_SETFL, FD_CLOEXEC);
		info->fd = pipefds[used_end];
		info->pid = childpid;
		popen_install(fp, info);
		fp->flags |= writing ? _FILE_WRITABLE : _FILE_READABLE;
		fp->fd = info->fd;
		return fp;
	}

	redirect_what = writing ? 0 /*stdin*/   : 1 /*stdout*/;
	used_end      = writing ? 0 /*reading*/ : 1 /*writing*/;
	unused_end    = writing ? 1 /*writing*/ : 0 /*reading*/;
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

int pclose(FILE* fp)
{
	return fclose(fp);
}

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    stdio/stdio.cpp
    Sets up stdin, stdout, stderr.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "fdio.h"

static struct fdio_state stdin_fdio = { NULL, 0 };
static struct fdio_state stdout_fdio = { NULL, 1 };
static struct fdio_state stderr_fdio = { NULL, 2 };

static FILE stdin_file;
static FILE stdout_file;
static FILE stderr_file;

extern "C" { FILE* stdin = &stdin_file; }
extern "C" { FILE* stdout = &stdout_file; }
extern "C" { FILE* stderr = &stderr_file; }

static void bootstrap_stdio(FILE* fp, struct fdio_state* fdio, int file_flags)
{
	fresetfile(fp);

	fp->flags |= file_flags;
	fp->user = fdio;
	fp->reopen_func = fdio_reopen;
	fp->read_func = fdio_read;
	fp->write_func = fdio_write;
	fp->seek_func = fdio_seek;
	fp->fileno_func = fdio_fileno;
	fp->close_func = fdio_close;

	fregister(fp);
}

extern "C" void init_stdio()
{
	bootstrap_stdio(stdin, &stdin_fdio, _FILE_READABLE);
	bootstrap_stdio(stdout, &stdout_fdio, _FILE_WRITABLE);
	bootstrap_stdio(stderr, &stderr_fdio, _FILE_WRITABLE);

	stderr->buffer_mode = _IONBF;
}

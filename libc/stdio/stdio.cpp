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
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "fdio.h"

static struct fdio_state stdin_fdio = { NULL, 0 };
static struct fdio_state stdout_fdio = { NULL, 1 };
static struct fdio_state stderr_fdio = { NULL, 2 };

extern "C" {

extern FILE __stdin_file;
extern FILE __stdout_file;
extern FILE __stderr_file;

FILE __stdin_file =
{
	/* buffersize = */ 0,
	/* buffer = */ NULL,
	/* user = */ &stdin_fdio,
	/* free_user = */ NULL,
	/* reopen_func = */ fdio_reopen,
	/* read_func = */ fdio_read,
	/* write_func = */ fdio_write,
	/* seek_func = */ fdio_seek,
	/* fileno_func = */ fdio_fileno,
	/* close_func = */ fdio_close,
	/* free_func = */ NULL,
	/* file_lock = */ PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
	/* fflush_indirect = */ NULL,
	/* buffer_free_indirect = */ NULL,
	/* prev = */ NULL,
	/* next = */ &__stdout_file,
	/* flags = */ _FILE_REGISTERED | _FILE_READABLE,
	/* buffer_mode = */ -1,
	/* offset_input_buffer = */ 0,
	/* amount_input_buffered = */ 0,
	/* amount_output_buffered = */ 0,
};

FILE __stdout_file
{
	/* buffersize = */ 0,
	/* buffer = */ NULL,
	/* user = */ &stdout_fdio,
	/* free_user = */ NULL,
	/* reopen_func = */ fdio_reopen,
	/* read_func = */ fdio_read,
	/* write_func = */ fdio_write,
	/* seek_func = */ fdio_seek,
	/* fileno_func = */ fdio_fileno,
	/* close_func = */ fdio_close,
	/* free_func = */ NULL,
	/* file_lock = */ PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
	/* fflush_indirect = */ NULL,
	/* buffer_free_indirect = */ NULL,
	/* prev = */ &__stdin_file,
	/* next = */ &__stderr_file,
	/* flags = */ _FILE_REGISTERED | _FILE_WRITABLE,
	/* buffer_mode = */ -1,
	/* offset_input_buffer = */ 0,
	/* amount_input_buffered = */ 0,
	/* amount_output_buffered = */ 0,
};

FILE __stderr_file
{
	/* buffersize = */ 0,
	/* buffer = */ NULL,
	/* user = */ &stderr_fdio,
	/* free_user = */ NULL,
	/* reopen_func = */ fdio_reopen,
	/* read_func = */ fdio_read,
	/* write_func = */ fdio_write,
	/* seek_func = */ fdio_seek,
	/* fileno_func = */ fdio_fileno,
	/* close_func = */ fdio_close,
	/* free_func = */ NULL,
	/* file_lock = */ PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
	/* fflush_indirect = */ NULL,
	/* buffer_free_indirect = */ NULL,
	/* prev = */ &__stdout_file,
	/* next = */ NULL,
	/* flags = */ _FILE_REGISTERED | _FILE_WRITABLE,
	/* buffer_mode = */ _IONBF,
	/* offset_input_buffer = */ 0,
	/* amount_input_buffered = */ 0,
	/* amount_output_buffered = */ 0,
};

FILE* stdin = &__stdin_file;
FILE* stdout = &__stdout_file;
FILE* stderr = &__stderr_file;

} /* extern "C" */

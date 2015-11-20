/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014, 2015.

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

    stdio/stdin.cpp
    Standard input.

*******************************************************************************/

#include <pthread.h>
#include <stdio.h>

#include "fdio.h"

static unsigned char stdin_buffer[BUFSIZ];
static struct fdio_state stdin_fdio = { NULL, 0 };
static FILE stdin_file =
{
	/* buffer = */ stdin_buffer,
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
	/* prev = */ NULL,
	/* next = */ NULL,
	/* flags = */ _FILE_REGISTERED | _FILE_READABLE,
	/* buffer_mode = */ -1,
	/* offset_input_buffer = */ 0,
	/* amount_input_buffered = */ 0,
	/* amount_output_buffered = */ 0,
};

extern "C" { FILE* const stdin = &stdin_file; }
extern "C" { FILE* volatile __stdin_used = &stdin_file; }

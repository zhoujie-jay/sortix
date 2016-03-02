/*
 * Copyright (c) 2011, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/stderr.c
 * Standard error.
 */

#include <pthread.h>
#include <stdio.h>

#include "fdio.h"

static FILE stderr_file =
{
	/* buffer = */ NULL,
	/* user = */ &stderr_file,
	/* free_user = */ NULL,
	/* read_func = */ NULL,
	/* write_func = */ fdio_write,
	/* seek_func = */ fdio_seek,
	/* close_func = */ fdio_close,
	/* free_func = */ NULL,
	/* file_lock = */ PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,
	/* fflush_indirect = */ NULL,
	/* prev = */ NULL,
	/* next = */ NULL,
	/* flags = */ _FILE_REGISTERED | _FILE_WRITABLE,
	/* buffer_mode = */ _IONBF,
	/* fd = */ 2,
	/* offset_input_buffer = */ 0,
	/* amount_input_buffered = */ 0,
	/* amount_output_buffered = */ 0,
};

FILE* const stderr = &stderr_file;
FILE* volatile __stderr_used = &stderr_file;

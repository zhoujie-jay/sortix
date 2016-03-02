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
 * stdio/fresetfile.c
 * After a FILE has been shut down, returns all fields to their default state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Note: This function preserves a few parts of the fields - this means that if
//       you are using this to reset a fresh FILE object, you should memset it
//       to zeroes first to avoid problems.
void fresetfile(FILE* fp)
{
	FILE* prev = fp->prev;
	FILE* next = fp->next;
	unsigned char* keep_buffer = fp->buffer;
	void* free_user = fp->free_user;
	void (*free_func)(void*, FILE*) = fp->free_func;
	int kept_flags = fp->flags & (_FILE_REGISTERED | 0);
	memset(fp, 0, sizeof(*fp));
	fp->buffer = keep_buffer;
	fp->file_lock = (pthread_mutex_t) PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	fp->flags = kept_flags;
	fp->buffer_mode = -1;
	fp->free_user = free_user;
	fp->free_func = free_func;
	fp->prev = prev;
	fp->next = next;
}

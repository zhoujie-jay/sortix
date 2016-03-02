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
 * FILE.h
 * The FILE structure from <stdio.h>
 */

#ifndef INCLUDE_FILE_H
#define INCLUDE_FILE_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <pthread.h>

#if !defined(BUFSIZ)
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif

#ifndef __FILE_defined
#define __FILE_defined
typedef struct __FILE FILE;
#endif

#define _FILE_REGISTERED (1<<0)
#define _FILE_BUFFER_MODE_SET (1<<1)
#define _FILE_LAST_WRITE (1<<2)
#define _FILE_LAST_READ (1<<3)
#define _FILE_BUFFER_OWNED (1<<4)
#define _FILE_STATUS_ERROR (1<<5)
#define _FILE_STATUS_EOF (1<<6)
#define _FILE_READABLE (1<<7)
#define _FILE_WRITABLE (1<<8)

#define _FILE_MAX_PUSHBACK 8

/* Note libc's declarations of stdin/stdout/stderr also needs to be changed if
   you make changes to this structure. */
struct __FILE
{
	unsigned char* buffer;
	void* user;
	void* free_user;
	ssize_t (*read_func)(void* user, void* ptr, size_t size);
	ssize_t (*write_func)(void* user, const void* ptr, size_t size);
	off_t (*seek_func)(void* user, off_t offset, int whence);
	int (*close_func)(void* user);
	void (*free_func)(void* free_user, FILE* fp);
	pthread_mutex_t file_lock;
	int (*fflush_indirect)(FILE*);
	FILE* prev;
	FILE* next;
	int flags;
	int buffer_mode;
	int fd;
	size_t offset_input_buffer;
	size_t amount_input_buffered;
	size_t amount_output_buffered;
};

/* Internally used by standard library. */
#if defined(__is_sortix_libc)
extern FILE* __first_file;
extern __pthread_mutex_t __first_file_lock;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

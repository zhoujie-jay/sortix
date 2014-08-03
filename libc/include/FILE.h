/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    FILE.h
    The FILE structure from <stdio.h>

*******************************************************************************/

#ifndef INCLUDE_FILE_H
#define INCLUDE_FILE_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <pthread.h>

__BEGIN_DECLS

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
typedef struct FILE FILE;
#endif

#define _FILE_REGISTERED (1<<0)
#define _FILE_BUFFER_MODE_SET (1<<1)
#define _FILE_LAST_WRITE (1<<2)
#define _FILE_LAST_READ (1<<3)
#define _FILE_AUTO_LOCK (1<<4)
#define _FILE_BUFFER_OWNED (1<<5)
#define _FILE_STATUS_ERROR (1<<6)
#define _FILE_STATUS_EOF (1<<7)
#define _FILE_READABLE (1<<8)
#define _FILE_WRITABLE (1<<9)

#define _FILE_MAX_PUSHBACK 8

/* Note stdio/stdio.cpp's declarations of stdin/stdout/stderr also needs to be
   changed if you make changes to this structure. */
struct FILE
{
	/* This is non-standard, but useful. If you allocate your own FILE and
	   register it with fregister, feel free to use modify the following members
	   to customize how it works. Don't call the functions directly, though, as
	   the standard library does various kinds of buffering and conversion. */
	size_t buffersize;
	unsigned char* buffer;
	void* user;
	void* free_user;
	int (*reopen_func)(void* user, const char* mode);
	ssize_t (*read_func)(void* user, void* ptr, size_t size);
	ssize_t (*write_func)(void* user, const void* ptr, size_t size);
	off_t (*seek_func)(void* user, off_t offset, int whence);
	int (*fileno_func)(void* user);
	int (*close_func)(void* user);
	void (*free_func)(void* free_user, struct FILE* fp);
	/* Application writers shouldn't use anything beyond this point. */
	pthread_mutex_t file_lock;
	int (*fflush_indirect)(FILE*);
	void (*buffer_free_indirect)(void*);
	struct FILE* prev;
	struct FILE* next;
	int flags;
	int buffer_mode;
	size_t offset_input_buffer;
	size_t amount_input_buffered;
	size_t amount_output_buffered;
};

/* Internally used by standard library. */
#if defined(__is_sortix_libc)
extern FILE* __first_file;
extern __pthread_mutex_t __first_file_lock;
#endif

__END_DECLS

#endif

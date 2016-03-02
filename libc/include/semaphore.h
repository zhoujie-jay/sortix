/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * semaphore.h
 * Semaphore API.
 */

#ifndef INCLUDE_SEMAPHORE_H
#define INCLUDE_SEMAPHORE_H

#include <sys/cdefs.h>

#include <sortix/timespec.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
#if defined(__is_sortix_libc)
	int value;
#else
	int __value;
#endif
} sem_t;

#define SEM_FAILED ((sem_t*) 0)

/*int sem_close(sem_t*);*/
int sem_destroy(sem_t*);
int sem_getvalue(sem_t* __restrict, int* __restrict);
int sem_init(sem_t*, int, unsigned int);
/*sem_t* sem_open(const char*, int, ...);*/
int sem_post(sem_t*);
int sem_timedwait(sem_t* __restrict, const struct timespec* __restrict);
int sem_trywait(sem_t*);
/*int sem_unlink(const char*);*/
int sem_wait(sem_t*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

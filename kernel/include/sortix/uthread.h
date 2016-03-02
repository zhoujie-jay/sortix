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
 * sortix/uthread.h
 * Header for user-space thread structures.
 */

#ifndef INCLUDE_SORTIX_UTHREAD_H
#define INCLUDE_SORTIX_UTHREAD_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define UTHREAD_FLAG_INITIAL (1UL << 0UL)

struct uthread
{
	struct uthread* uthread_pointer;
	size_t uthread_size;
	unsigned long uthread_flags;
	void* tls_master_mmap;
	size_t tls_master_size;
	size_t tls_master_align;
	void* tls_mmap;
	size_t tls_size;
	void* stack_mmap;
	size_t stack_size;
	void* arg_mmap;
	size_t arg_size;
	size_t __uthread_reserved[4];
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

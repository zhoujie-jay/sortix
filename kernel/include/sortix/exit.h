/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/exit.h
 * Flags and constants related to process and thread exiting.
 */

#ifndef INCLUDE_SORTIX_EXIT_H
#define INCLUDE_SORTIX_EXIT_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

struct exit_thread
{
	void* unmap_from;
	size_t unmap_size;
	void* zero_from;
	size_t zero_size;
	void* tls_unmap_from;
	size_t tls_unmap_size;
	unsigned long reserved[2];
};

#define EXIT_THREAD_ONLY_IF_OTHERS (1<<0)
#define EXIT_THREAD_UNMAP (1<<1)
#define EXIT_THREAD_ZERO (1<<2)
#define EXIT_THREAD_TLS_UNMAP (1<<3)
#define EXIT_THREAD_PROCESS (1<<4)
#define EXIT_THREAD_DUMP_CORE (1<<5)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

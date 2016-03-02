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
 * DIR.h
 * The DIR structure from <dirent.h>
 */

#ifndef INCLUDE_DIR_H
#define INCLUDE_DIR_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

struct dirent;

#ifndef __DIR_defined
#define __DIR_defined
typedef struct __DIR DIR;
#endif

#define _DIR_REGISTERED (1<<0)
#define _DIR_ERROR (1<<1)
#define _DIR_EOF (1<<2)

struct __DIR
{
	struct dirent* entry;
	size_t size;
	int fd;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

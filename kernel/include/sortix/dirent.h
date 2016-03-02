/*
 * Copyright (c) 2012, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * sortix/dirent.h
 * Format of directory entries.
 */

#ifndef INCLUDE_SORTIX_DIRENT_H
#define INCLUDE_SORTIX_DIRENT_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <stdint.h>

#include <sortix/__/dirent.h>
#include <sortix/__/dt.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dev_t_defined
#define __dev_t_defined
typedef __dev_t dev_t;
#endif

#ifndef __ino_t_defined
#define __ino_t_defined
typedef __ino_t ino_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#if __USE_SORTIX
#define DT_UNKNOWN __DT_UNKNOWN
#define DT_BLK __DT_BLK
#define DT_CHR __DT_CHR
#define DT_DIR __DT_DIR
#define DT_FIFO __DT_FIFO
#define DT_LNK __DT_LNK
#define DT_REG __DT_REG
#define DT_SOCK __DT_SOCK
#endif

#if __USE_SORTIX
#define IFTODT(x) __IFTODT(x)
#define DTTOIF(x) __DTTOIF(x)
#endif

struct dirent
{
	size_t d_reclen;
	size_t d_namlen;
	ino_t d_ino;
	dev_t d_dev;
	unsigned char d_type;
	__extension__ char d_name[];
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

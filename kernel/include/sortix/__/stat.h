/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/__/stat.h
 * Defines the struct stat used for file meta-information and other useful
 * macros and values relating to values stored in it.
 */

#ifndef INCLUDE_SORTIX____STAT_H
#define INCLUDE_SORTIX____STAT_H

#include <sys/cdefs.h>

#include <sortix/__/dt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __S_IFMT_SHIFT 12
#define __S_IFMT_MASK __DT_BITS

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

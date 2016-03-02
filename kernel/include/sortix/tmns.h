/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * sortix/tmns.h
 * Declarations for the kernel time interfaces.
 */

#ifndef INCLUDE_SORTIX_TMNS_H
#define INCLUDE_SORTIX_TMNS_H

#include <sys/cdefs.h>

#include <sortix/timespec.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tmns
{
	struct timespec tmns_utime;
	struct timespec tmns_stime;
	struct timespec tmns_cutime;
	struct timespec tmns_cstime;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

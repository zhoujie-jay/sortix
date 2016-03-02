/*
 * Copyright (c) 2011, 2014 Jonas 'Sortie' Termansen.
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
 * error.h
 * Error reporting functions.
 */

#ifndef INCLUDE_ERROR_H
#define INCLUDE_ERROR_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

void gnu_error(int status, int errnum, const char* format, ...)
	__attribute__((__format__(__printf__, 3, 4)));
void error(int status, int errnum, const char* format, ...) __asm__ ("gnu_error")
	__attribute__((__format__(__printf__, 3, 4)));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

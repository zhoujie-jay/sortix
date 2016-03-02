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
 * stdio_ext.h
 * Extensions to stdio.h introduced by Solaris and glibc.
 */

#ifndef _STDIO_EXT_H
#define _STDIO_EXT_H 1

#include <sys/cdefs.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t __fbufsize(FILE* fp);
size_t __fpending(FILE* fp);
void __fpurge(FILE* fp);
int __freadable(FILE* fp);
int __freading(FILE* fp);
void __fseterr(FILE* fp);
int __fwritable(FILE* fp);
int __fwriting(FILE* fp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

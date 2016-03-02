/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * setjmp.h
 * Stack environment declarations.
 */

#ifndef _SETJMP_H
#define _SETJMP_H 1

#include <sys/cdefs.h>

#include <sortix/__/sigset.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__x86_64__)
typedef unsigned long sigjmp_buf[8 + 1 + __SIGSET_NUM_SIGNALS / (sizeof(unsigned long int) * 8)];
#elif defined(__i386__)
typedef unsigned long sigjmp_buf[6 + 1 + __SIGSET_NUM_SIGNALS / (sizeof(unsigned long int) * 8)];
#else
#error "You need to implement sigjmp_buf on your CPU"
#endif

typedef sigjmp_buf jmp_buf;

void longjmp(jmp_buf, int);
int setjmp(jmp_buf);
void siglongjmp(sigjmp_buf, int);
int sigsetjmp(sigjmp_buf, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

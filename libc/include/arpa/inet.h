/*
 * Copyright (c) 2013, 2014, 2016 Jonas 'Sortie' Termansen.
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
 * arpa/inet.h
 * Definitions for internet operations.
 */

#ifndef INCLUDE_ARPA_INET_H
#define INCLUDE_ARPA_INET_H

#include <sys/cdefs.h>

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Functions from POSIX that is considered obsolete due to bad design. */
#if __USE_SORTIX || __USE_POSIX
in_addr_t inet_addr(const char*);
char* inet_ntoa(struct in_addr);
#endif

/* Functions from POSIX. */
#if __USE_SORTIX || __USE_POSIX
const char* inet_ntop(int, const void* __restrict, char* __restrict, socklen_t);
int inet_pton(int, const char* __restrict, void* __restrict);
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
/* TODO: int inet_aton(const char*, struct in_addr*); */
/* TODO: char* inet_neta(in_addr_t, char*, size_t); */
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

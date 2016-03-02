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
 * sortix/socket.h
 * Declarations for socket types and other flags.
 */

#ifndef SORTIX_INCLUDE_SOCKET_H
#define SORTIX_INCLUDE_SOCKET_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: Nicely wrap this in an enum, as in glibc's header? */
#define SOCK_TYPE_MASK ((1<<20)-1)
#define SOCK_RAW 0 /* Will Sortix support this? */
#define SOCK_DGRAM 1
#define SOCK_SEQPACKET 2
#define SOCK_STREAM 3

#define SOCK_NONBLOCK (1<<20)
#define SOCK_CLOEXEC (1<<21)
#define SOCK_CLOFORK (1<<22)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

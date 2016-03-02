/*
 * Copyright (c) 2012, 2015 Jonas 'Sortie' Termansen.
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
 * sortix/poll.h
 * Interface for waiting on file descriptor events.
 */

#ifndef INCLUDE_SORTIX_POLL_H
#define INCLUDE_SORTIX_POLL_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __SIZE_TYPE__ nfds_t;

struct pollfd
{
	int fd;
	short events;
	short revents;
};

#define POLLERR (1<<0)
#define POLLHUP (1<<1)
#define POLLNVAL (1<<2)

#define POLLIN (1<<3)
#define POLLRDNORM (1<<4)
#define POLLRDBAND (1<<5)
#define POLLPRI (1<<6)
#define POLLOUT (1<<7)
#define POLLWRNORM (1<<8)
#define POLLWRBAND (1<<9)

#define POLL__ONLY_REVENTS (POLLERR | POLLHUP | POLLNVAL)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

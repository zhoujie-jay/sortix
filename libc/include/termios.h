/*
 * Copyright (c) 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * termios.h
 * Defines values for termios.
 */

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef INCLUDE_TERMIOS_H
#define INCLUDE_TERMIOS_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/termios.h>
#if __USE_SORTIX
#include <sortix/winsize.h>
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#if __USE_SORTIX
#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif
#endif

#if __USE_SORTIX
#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

speed_t cfgetispeed(const struct termios*);
speed_t cfgetospeed(const struct termios*);
int cfsetispeed(struct termios*, speed_t);
int cfsetospeed(struct termios*, speed_t);
int tcdrain(int);
int tcflow(int, int);
int tcflush(int, int);
int tcgetattr(int, struct termios*);
pid_t tcgetsid(int);
int tcsendbreak(int, int);
int tcsetattr(int, int, const struct termios*);

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
ssize_t tcgetblob(int, const char*, void*, size_t);
ssize_t tcsetblob(int, const char*, const void*, size_t);
int tcgetwincurpos(int, struct wincurpos*);
int tcgetwinsize(int, struct winsize*);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * fcntl.h
 * File control options.
 */

#ifndef INCLUDE_FCNTL_H
#define INCLUDE_FCNTL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sys/stat.h>

#include <sortix/fcntl.h>
#include <sortix/seek.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The kernel would like to simply deal with one bit for each base access mode,
   but using the traditional names O_RDONLY, O_WRONLY and O_RDWR for this would
   be weird, so it uses O_READ and O_WRITE bits instead. However, we provide the
   traditional names here instead to remain compatible. */
#define O_RDONLY O_READ
#define O_WRONLY O_WRITE
#define O_RDWR (O_READ | O_WRITE)

/* Backwards compatibility with existing systems that call it O_CREAT. */
#define O_CREAT O_CREATE

/* TODO: POSIX_FADV_* missing here */

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

int creat(const char* path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int open(const char* path, int oflag, ...);
int openat(int fd, const char* path, int oflag, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

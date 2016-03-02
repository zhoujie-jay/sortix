/*
 * Copyright (c) 2011, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * stdio/fdio.h
 * Handles the file descriptor backend for the FILE* API.
 */

#ifndef STDIO_FDIO_H
#define STDIO_FDIO_H

#include <sys/cdefs.h>

#include <sys/stat.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t fdio_read(void* user, void* ptr, size_t size);
ssize_t fdio_write(void* user, const void* ptr, size_t size);
off_t fdio_seek(void* user, off_t offset, int whence);
int fdio_close(void* user);

bool fdio_install_fd(FILE* fp, int fd, const char* mode);
bool fdio_install_path(FILE* fp, const char* path, const char* mode);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

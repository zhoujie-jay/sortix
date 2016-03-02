/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * dirent.h
 * Format of directory entries.
 */

#ifndef INCLUDE_DIRENT_H
#define INCLUDE_DIRENT_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/dirent.h>

#if defined(__is_sortix_libc)
#include <DIR.h>
#endif

#ifndef __DIR_defined
#define __DIR_defined
typedef struct __DIR DIR;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int closedir(DIR*);
DIR* opendir(const char*);
struct dirent* readdir(DIR*);
void rewinddir(DIR*);

#if __USE_SORTIX || __USE_XOPEN
/* TODO: seekdir */
/* TODO: telldir */
#endif

/* Functions from POSIX 1995. */
#if 199506L <= __USE_POSIX
/* readdir_r will not be implemented. */
#endif

/* Functions from POSIX 2008. */
#if __USE_SORTIX || 200809L <= __USE_POSIX
int alphasort(const struct dirent**, const struct dirent**);
int dirfd(DIR*);
DIR* fdopendir(int);
int scandir(const char*, struct dirent***, int (*)(const struct dirent*),
            int (*)(const struct dirent**, const struct dirent**));
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
int versionsort(const struct dirent**, const struct dirent**);
#endif

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
int alphasort_r(const struct dirent**, const struct dirent**, void*);
int dscandir_r(DIR*, struct dirent***,
               int (*)(const struct dirent*, void*),
               void*,
               int (*)(const struct dirent**, const struct dirent**, void*),
               void*);
int versionsort_r(const struct dirent**, const struct dirent**, void*);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

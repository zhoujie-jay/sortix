/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * fstab.h
 * Filesystem table.
 */

#ifndef INCLUDE_FSTAB_H
#define INCLUDE_FSTAB_H

#include <sys/cdefs.h>

#ifndef __FILE_defined
#define __FILE_defined
typedef struct __FILE FILE;
#endif

struct fstab
{
	char* fs_spec;
	char* fs_file;
	char* fs_vfstype;
	char* fs_mntops;
	char* fs_type;
	int fs_freq;
	int fs_passno;
};

#define	FSTAB_RO "ro"
#define	FSTAB_RQ "rq"
#define	FSTAB_RW "rw"
#define	FSTAB_SW "sw"
#define	FSTAB_XX "xx"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__is_sortix_libc)
extern FILE* __fstab_file;
#endif

void endfsent(void);
struct fstab* getfsent(void);
struct fstab* getfsfile(const char*);
struct fstab* getfsspec(const char*);
int scanfsent(char*, struct fstab*);
int setfsent(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

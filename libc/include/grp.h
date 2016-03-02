/*
 * Copyright (c) 2013, 2015 Jonas 'Sortie' Termansen.
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
 * grp.h
 * Group database.
 */

#ifndef INCLUDE_GRP_H
#define INCLUDE_GRP_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FILE_defined
#define __FILE_defined
typedef struct __FILE FILE;
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

struct group
{
	gid_t gr_gid;
	char** gr_mem;
	char* gr_name;
	char* gr_passwd;
};

#if defined(__is_sortix_libc)
extern FILE* __grp_file;
#endif

void endgrent(void);
struct group* fgetgrent(FILE*);
int fgetgrent_r(FILE* __restrict, struct group* __restrict, char* __restrict,
                size_t, struct group** __restrict);
struct group* getgrent(void);
int getgrent_r(struct group* __restrict, char* __restrict, size_t,
               struct group** __restrict);
struct group* getgrgid(gid_t);
int getgrgid_r(gid_t, struct group* __restrict, char* __restrict, size_t,
               struct group** __restrict);
struct group* getgrnam(const char*);
int getgrnam_r(const char*, struct group*, char*, size_t, struct group**);
FILE* opengr(void);
void setgrent(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

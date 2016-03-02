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
 * pwd.h
 * User database.
 */

#ifndef INCLUDE_PWD_H
#define INCLUDE_PWD_H

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

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

struct passwd
{
	uid_t pw_uid;
	gid_t pw_gid;
	char* pw_dir;
	char* pw_gecos;
	char* pw_name;
	char* pw_passwd;
	char* pw_shell;
};

#if defined(__is_sortix_libc)
extern FILE* __pwd_file;
#endif

int bcrypt_newhash(const char*, int, char*, size_t);
int bcrypt_checkpass(const char*, const char*);
void endpwent(void);
struct passwd* fgetpwent(FILE*);
int fgetpwent_r(FILE* __restrict, struct passwd* __restrict, char* __restrict,
                size_t, struct passwd** __restrict);
struct passwd* getpwent(void);
int getpwent_r(struct passwd* __restrict, char* __restrict, size_t,
               struct passwd** __restrict);
struct passwd* getpwnam(const char*);
int getpwnam_r(const char* __restrict, struct passwd* __restrict,
               char* __restrict, size_t, struct passwd** __restrict);
struct passwd* getpwuid(uid_t);
int getpwuid_r(uid_t, struct passwd* __restrict, char* __restrict, size_t,
               struct passwd** __restrict);
FILE* openpw(void);
void setpwent(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

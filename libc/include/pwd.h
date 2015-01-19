/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    pwd.h
    User database.

*******************************************************************************/

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

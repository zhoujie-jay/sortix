/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

__BEGIN_DECLS

@include(FILE.h)
@include(gid_t.h)
@include(size_t.h)
@include(uid_t.h)

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

#if __is_sortix_libc
extern FILE* __pwd_file;
#endif

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

__END_DECLS

#endif

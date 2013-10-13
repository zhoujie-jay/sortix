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

    grp.h
    Group database.

*******************************************************************************/

#ifndef INCLUDE_GRP_H
#define INCLUDE_GRP_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

@include(FILE.h)
@include(gid_t.h)
@include(size_t.h)

struct group
{
	gid_t gr_gid;
	char** gr_mem;
	char* gr_name;
	char* gr_passwd;
};

#if __is_sortix_libc
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

__END_DECLS

#endif

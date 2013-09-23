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

#include <features.h>

#include <sys/__/types.h>

__BEGIN_DECLS

@include(gid_t.h)
@include(uid_t.h)
@include(size_t.h)

#define _PASSWD_BUFFER_SIZE 64

struct passwd
{
	char pw_name[_PASSWD_BUFFER_SIZE];
	char pw_dir[_PASSWD_BUFFER_SIZE];
	char pw_shell[_PASSWD_BUFFER_SIZE];
	char pw_gecos[_PASSWD_BUFFER_SIZE];
	uid_t pw_uid;
	gid_t pw_gid;
};

void endpwent(void);
struct passwd* getpwent(void);
struct passwd* getpwnam(const char*);
int getpwnam_r(const char*, struct passwd*, char*, size_t, struct passwd**);
struct passwd* getpwuid(uid_t);
int getpwuid_r(uid_t, struct passwd *, char*, size_t, struct passwd**);
void setpwent(void);

__END_DECLS

#endif

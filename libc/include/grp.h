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

#include <features.h>

#include <sys/__/types.h>

__BEGIN_DECLS

@include(gid_t.h)
@include(uid_t.h)
@include(size_t.h)

#define _GROUP_BUFFER_SIZE 64

struct group
{
	char gr_name[_GROUP_BUFFER_SIZE];
	gid_t gr_gid;
	char** gr_mem;
};

struct group* getgrgid(gid_t gid);
struct group* getgrnam(const char* name);

__END_DECLS

#endif

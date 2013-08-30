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

    sys/resource.h
    Resource limits and operations.

*******************************************************************************/

#ifndef INCLUDE_SYS_RESOURCE_H
#define INCLUDE_SYS_RESOURCE_H

#include <features.h>
#include <sortix/resource.h>

__BEGIN_DECLS

@include(id_t.h)
@include(pid_t.h)

int getpriority(int, id_t);
int getrlimit(int, struct rlimit*);
int prlimit(pid_t, int, const struct rlimit*, struct rlimit*);
int setpriority(int, id_t, int);
int setrlimit(int, const struct rlimit*);

__END_DECLS

#endif
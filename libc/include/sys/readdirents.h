/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    sys/readdirents.h
    Allows reading directory entries directly from a file descriptor.

*******************************************************************************/

#ifndef _SYS_READDIRENTS_H
#define _SYS_READDIRENTS_H 1

#include <features.h>

#include <sys/__/types.h>

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <sortix/dirent.h>

__BEGIN_DECLS

@include(size_t.h)

ssize_t readdirents(int fd, struct kernel_dirent* dirent, size_t size);

__END_DECLS

#endif

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    sys/types.h
    Data types.

*******************************************************************************/

// TODO: Make this header comply with POSIX-1.2008

#ifndef INCLUDE_SYS_TYPES_H
#define INCLUDE_SYS_TYPES_H

#include <features.h>

#include <sys/__/types.h>

__BEGIN_DECLS

@include(blkcnt_t.h)
@include(blksize_t.h)
@include(clock_t.h)
@include(clockid_t.h)
@include(dev_t.h)
/* TODO: fsblkcnt_t */
/* TODO: fsfilcnt_t */
@include(gid_t.h)
@include(id_t.h)
@include(ino_t.h)
/* TODO: key_t */
@include(mode_t.h)
@include(nlink_t.h)
@include(off_t.h)
@include(pid_t.h)
@include(size_t.h)
@include(ssize_t.h)
@include(suseconds_t.h)
@include(time_t.h)
@include(timer_t.h)
/* TODO: trace*_t */
@include(uid_t.h)
@include(useconds_t.h)

#if !defined(__is_sortix_kernel)
/* TODO: pthread*_t */
#endif

__END_DECLS

#endif

/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	types.h
	Data types.

******************************************************************************/

// TODO: Make this header comply with POSIX-1.2008

#ifndef	_STAT_H
#define	_STAT_H 1

#include <features.h>

__BEGIN_DECLS

@include(blkcnt_t.h)
@include(blksize_t.h)
@include(clock_t.h)
/* TODO: clockid_t */
@include(dev_t.h)
/* TODO: fsblkcnt_t */
/* TODO: fsfilcnt_t */
@include(gid_t.h)
/* TODO: id_t */
@include(ino_t.h)
/* TODO: key_t */
@include(mode_t.h)
@include(nlink_t.h)
@include(off_t.h)
@include(pid_t.h)
/* TODO: pthread*_t */
@include(size_t.h)
@include(ssize_t.h)
@include(suseconds_t.h)
@include(time_t.h)
/* TODO: timer_t */
/* TODO: trace*_t */
@include(uid_t.h)
@include(useconds_t.h)

__END_DECLS

#endif


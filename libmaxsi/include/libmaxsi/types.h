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
	Declares the required datatypes based on the kernel's configuration.

******************************************************************************/

#ifndef LIBMAXSI_TYPES_H
#define LIBMAXSI_TYPES_H

#include <sortix/bits.h>

@include(NULL.h)

@include(intn_t.h)

@include(wint_t.h)
@include(id_t.h)
@include(gid_t.h)
@include(pid_t.h)
@include(uid_t.h)
@include(locale_t.h)
@include(mode_t.h)
@include(off_t.h)
@include(gid_t.h)
@include(ino_t.h)
@include(nlink_t.h)
@include(blksize_t.h)
@include(blkcnt_t.h)

@include(va_list.h)

// Maxsi datatype extentions.
typedef __nat nat;
typedef uint8_t byte;
typedef uintptr_t addr_t;

#endif


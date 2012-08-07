/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/fcntl.h
    Declares various constants related to opening files.

*******************************************************************************/

#ifndef SORTIX_FCNTL_H
#define SORTIX_FCNTL_H

#include <features.h>

__BEGIN_DECLS

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 3
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#define O_EXEC 4
#define O_SEARCH 5
#define O_LOWERFLAGS 0x7
#define O_APPEND (1<<3)
#define O_CLOEXEC (1<<4)
#define O_CREAT (1<<5)
#define O_DIRECTORY (1<<6)
#define O_EXCL (1<<7)
#define O_TRUNC (1<<8)
#define O_CLOFORK (1<<9)

#define FD_CLOEXEC (1<<0)
#define FD_CLOFORK (1<<1)

#define __FD_ALLOWED_FLAGS (FD_CLOEXEC | FD_CLOFORK)

#define F_SETFD 0
#define F_GETFD 1
#define F_SETFL 2
#define F_GETFL 3

#define AT_FDCWD (-100)

__END_DECLS

#endif

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    ioleast.h
    Versions of {,p}{read,write} that don't return until it has returned as much
    data as requested, end of file, or an error occurs. This is sometimes needed
    as read(2) and write(2) is not always guaranteed to fill up the entire
    buffer or write it all.

*******************************************************************************/

#ifndef INCLUDE_IOLEAST_H
#define INCLUDE_IOLEAST_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

size_t preadall(int fd, void* buf, size_t count, off_t off);
size_t preadleast(int fd, void* buf, size_t least, size_t max, off_t off);
size_t pwriteall(int fd, const void* buf, size_t count, off_t off);
size_t pwriteleast(int fd, const void* buf, size_t least, size_t max, off_t off);
size_t readall(int fd, void* buf, size_t count);
size_t readleast(int fd, void* buf, size_t least, size_t max);
size_t writeall(int fd, const void* buf, size_t count);
size_t writeleast(int fd, const void* buf, size_t least, size_t max);

__END_DECLS

#endif

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

    sys/mman.h
    Memory management declarations.

*******************************************************************************/

#ifndef INCLUDE_SYS_MMAN_H
#define INCLUDE_SYS_MMAN_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/mman.h>

__BEGIN_DECLS

#ifndef __mode_t_defined
#define __mode_t_defined
typedef __mode_t mode_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

void* mmap(void*, size_t, int, int, int, off_t);
int mprotect(const void*, size_t, int);
int munmap(void*, size_t);

__END_DECLS

#endif

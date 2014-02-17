/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/uthread.h
    Header for user-space thread structures.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_UTHREAD_H
#define INCLUDE_SORTIX_UTHREAD_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

__BEGIN_DECLS

#define UTHREAD_FLAG_INITIAL (1UL << 0UL)

struct uthread
{
	struct uthread* uthread_pointer;
	size_t uthread_size;
	unsigned long uthread_flags;
	void* tls_master_mmap;
	size_t tls_master_size;
	size_t tls_master_align;
	void* tls_mmap;
	size_t tls_size;
	void* stack_mmap;
	size_t stack_size;
	void* arg_mmap;
	size_t arg_size;
	size_t __uthread_reserved[4];
};

__END_DECLS

#endif

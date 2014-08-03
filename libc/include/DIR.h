/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    DIR.h
    The DIR structure from <dirent.h>

*******************************************************************************/

#ifndef INCLUDE_DIR_H
#define INCLUDE_DIR_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <pthread.h>

__BEGIN_DECLS

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

struct dirent;

#ifndef __DIR_defined
#define __DIR_defined
typedef struct DIR DIR;
#endif

#define _DIR_REGISTERED (1<<0)
#define _DIR_ERROR (1<<1)
#define _DIR_EOF (1<<2)

struct DIR
{
	void* user;
	int (*read_func)(void* user, struct dirent* dirent, size_t* size);
	int (*rewind_func)(void* user);
	int (*fd_func)(void* user);
	int (*close_func)(void* user);
	void (*free_func)(DIR* dir);
	/* Application writers shouldn't use anything beyond this point. */
	DIR* prev;
	DIR* next;
	struct dirent* entry;
	size_t entrysize;
	int flags;
};

#if defined(__is_sortix_libc)
extern DIR* __first_dir;
extern __pthread_mutex_t __first_dir_lock;
#endif

__END_DECLS

#endif

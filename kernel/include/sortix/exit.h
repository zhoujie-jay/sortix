/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    sortix/exit.h
    Flags and constants related to process and thread exiting.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_EXIT_H
#define INCLUDE_SORTIX_EXIT_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

struct exit_thread
{
	void* unmap_from;
	size_t unmap_size;
	void* zero_from;
	size_t zero_size;
	unsigned long reserved[4];
};

#define EXIT_THREAD_ONLY_IF_OTHERS (1<<0)
#define EXIT_THREAD_UNMAP (1<<1)
#define EXIT_THREAD_ZERO (1<<2)

__END_DECLS

#endif

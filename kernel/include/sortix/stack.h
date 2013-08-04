/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

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

    sortix/stack.h
    Declares stack_t and associated flags.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_STACK_H
#define INCLUDE_SORTIX_STACK_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#define SS_ONSTACK (1<<0)
#define SS_DISABLE (1<<1)

#define __SS_SUPPORTED_FLAGS (SS_ONSTACK | SS_DISABLE)

typedef struct
{
	void* ss_sp;
	size_t ss_size;
	int ss_flags;
} stack_t;

__END_DECLS

#endif

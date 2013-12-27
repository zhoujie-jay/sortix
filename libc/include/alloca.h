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

    alloca.h
    Stack-based memory allocation.

*******************************************************************************/

#ifndef INCLUDE_ALLOCA_H
#define INCLUDE_ALLOCA_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

/* If somehow another declaration of alloca happened. This shouldn't happen, but
   glibc does this and we might as well do it also. */
#undef alloca

/* Declare a function prototype despite that there really is no alloca function
   in Sortix. The compiler will normally be run with -fbuiltin and simply use
   the compiler builtin as intended. */
void* alloca(size_t);

/* In case the compilation is run with -fno-builtin, we'll call the builtin
   function directly, since there is no alloca function in the first place. You
   can simply undef this if you want the real prototype, which will give a link
   error if -fno-builtin is passed. */
#define alloca(size) __builtin_alloca(size)

__END_DECLS

#endif

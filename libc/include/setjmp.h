/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    setjmp.h
    Stack environment declarations.

*******************************************************************************/

#ifndef _SETJMP_H
#define _SETJMP_H 1

#include <sys/cdefs.h>

__BEGIN_DECLS

#if defined(__x86_64__)
typedef unsigned long jmp_buf[8];
#elif defined(__i386__)
typedef unsigned long jmp_buf[6];
#else
#error "You need to implement jmp_buf on your CPU"
#endif

void longjmp(jmp_buf env, int val);
int setjmp(jmp_buf env);

__END_DECLS

#endif

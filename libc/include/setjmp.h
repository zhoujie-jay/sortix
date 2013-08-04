/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

#include <sortix/__/sigset.h>

__BEGIN_DECLS

#if defined(__x86_64__)
typedef unsigned long sigjmp_buf[8 + 1 + __SIGSET_NUM_SIGNALS / (sizeof(unsigned long int) * 8)];
#elif defined(__i386__)
typedef unsigned long sigjmp_buf[6 + 1 + __SIGSET_NUM_SIGNALS / (sizeof(unsigned long int) * 8)];
#else
#error "You need to implement sigjmp_buf on your CPU"
#endif

typedef sigjmp_buf jmp_buf;

void longjmp(jmp_buf, int);
int setjmp(jmp_buf);
void siglongjmp(sigjmp_buf, int);
int sigsetjmp(sigjmp_buf, int);

__END_DECLS

#endif

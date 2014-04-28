/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    sys/syscall.h
    Functions and macros for declaring system call stubs.

*******************************************************************************/

#if defined(__is_sortix_kernel)
#error "This file is part of user-space and should not be built in kernel mode"
#endif

#ifndef INCLUDE_SYS_SYSCALL_H
#define INCLUDE_SYS_SYSCALL_H

#include <sys/cdefs.h>

#include <sortix/syscallnum.h>

/* Expand a macro and convert it to string. */
#define SYSCALL_STRINGIFY_EXPAND(foo) #foo

/* Implement the body of a function that selects the right system call and
   jumps into the generic implementation of system calls. */
#if defined(__i386__)

#define SYSCALL_FUNCTION_BODY(syscall_index) \
"	mov $" SYSCALL_STRINGIFY_EXPAND(syscall_index) ", %eax\n" \
"	jmp asm_syscall\n"

#elif defined(__x86_64__)

#define SYSCALL_FUNCTION_BODY(syscall_index) \
"	mov $" SYSCALL_STRINGIFY_EXPAND(syscall_index) ", %rax\n" \
"	jmp asm_syscall\n"

#else

#error "Provide an implementation for your platform."

#endif

/* Create a function that selects the right system call and jumps into the
   generic implementation of system calls. */
#define SYSCALL_FUNCTION(syscall_name, syscall_index) \
__asm__("\n" \
".pushsection .text\n" \
".type " #syscall_name ", @function\n" \
#syscall_name ":\n" \
SYSCALL_FUNCTION_BODY(syscall_index) \
".size " #syscall_name ", . - " #syscall_name "\n" \
".popsection\n" \
);

/* Create a function that performs the system call by injecting the right
   instructions into the compiler assembly output. Then provide a declaration of
   the function that looks just like the caller wants it. */
#define DEFINE_SYSCALL(syscall_type, syscall_name, syscall_index, syscall_formals) \
SYSCALL_FUNCTION(syscall_name, syscall_index) \
__BEGIN_DECLS \
syscall_type syscall_name syscall_formals; \
__END_DECLS \

/* System call accepting no parameters. */
#define DEFN_SYSCALL0(type, fn, num) \
DEFINE_SYSCALL(type, fn, num, ())

/* System call accepting 1 parameter. */
#define DEFN_SYSCALL1(type, fn, num, t1) \
DEFINE_SYSCALL(type, fn, num, (t1 p1))

/* System call accepting 2 parameters. */
#define DEFN_SYSCALL2(type, fn, num, t1, t2) \
DEFINE_SYSCALL(type, fn, num, (t1 p1, t2 p2))

/* System call accepting 3 parameters. */
#define DEFN_SYSCALL3(type, fn, num, t1, t2, t3) \
DEFINE_SYSCALL(type, fn, num, (t1 p1, t2 p2, t3 p3))

/* System call accepting 4 parameters. */
#define DEFN_SYSCALL4(type, fn, num, t1, t2, t3, t4) \
DEFINE_SYSCALL(type, fn, num, (t1 p1, t2 p2, t3 p3, t4 p4))

/* System call accepting 5 parameters. */
#define DEFN_SYSCALL5(type, fn, num, t1, t2, t3, t4, t5) \
DEFINE_SYSCALL(type, fn, num, (t1 p1, t2 p2, t3 p3, t4 p4, t5 p5))

#endif

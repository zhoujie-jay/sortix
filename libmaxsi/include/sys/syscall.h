/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	sys/syscall.h
	Functions and macros for declaring system call stubs.

*******************************************************************************/

#if defined(SORTIX_KERNEL)
#error This part of libc should not be built in kernel mode
#endif

#ifndef	_SYS_SYSCALL_H
#define	_SYS_SYSCALL_H 1

#include <features.h>
#include <sortix/syscallnum.h>
#include <errno.h> /* Should not be exposed here; use an eventual __errno.h. */

__BEGIN_DECLS

#define DECL_SYSCALL0(type,fn) type fn();
#define DECL_SYSCALL1(type,fn,p1) type fn(p1);
#define DECL_SYSCALL2(type,fn,p1,p2) type fn(p1,p2);
#define DECL_SYSCALL3(type,fn,p1,p2,p3) type fn(p1,p2,p3);
#define DECL_SYSCALL4(type,fn,p1,p2,p3,p4) type fn(p1,p2,p3,p4);
#define DECL_SYSCALL5(type,fn,p1,p2,p3,p4,p5) type fn(p1,p2,p3,p4,p5);

// System call functions for i386. (IA-32)
#if defined(__i386__)

#define DEFN_SYSCALL0(type, fn, num) \
inline type fn() \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL1(type, fn, num, P1) \
inline type fn(P1 p1) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL2(type, fn, num, P1, P2) \
inline type fn(P1 p1, P2 p2) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL3(type, fn, num, P1, P2, P3) \
inline type fn(P1 p1, P2 p2, P3 p3) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2), "d" ((unsigned long)p3)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL4(type, fn, num, P1, P2, P3, P4) \
inline type fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2), "d" ((unsigned long)p3), "D" ((unsigned long)p4)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL5(type, fn, num, P1, P2, P3, P4, P5) \
inline type fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2), "d" ((unsigned long)p3), "D" ((unsigned long)p4), "S" ((unsigned long)p5)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL0_VOID(fn, num) \
inline void fn() \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL1_VOID(fn, num, P1) \
inline void fn(P1 p1) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL2_VOID(fn, num, P1, P2) \
inline void fn(P1 p1, P2 p2) \
{ \
	unsigned long a; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL3_VOID(fn, num, P1, P2, P3) \
inline void fn(P1 p1, P2 p2, P3 p3) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2), "d" ((unsigned long)p3)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL4_VOID(fn, num, P1, P2, P3, P4) \
inline void fn(P1 p1, P2 p2, P3 p3, P4 p4) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2), "d" ((unsigned long)p3), "D" ((unsigned long)p4)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL5_VOID(fn, num, P1, P2, P3, P4, P5) \
inline void fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((unsigned long)p1), "c" ((unsigned long)p2), "d" ((unsigned long)p3), "D" ((unsigned long)p4), "S" ((unsigned long)p5)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

// System call functions for x86_64. (amd64)
#elif defined(__x86_64__)

// TODO: Make these inline - though that requires a move advanced inline
// assembly stub to force parameters into the right registers.

#define DEFN_SYSCALL0(type, fn, num) \
type fn() \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL1(type, fn, num, P1) \
type fn(P1) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL2(type, fn, num, P1, P2) \
type fn(P1, P2) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL3(type, fn, num, P1, P2, P3) \
type fn(P1, P2, P3) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL4(type, fn, num, P1, P2, P3, P4) \
type fn(P1, P2, P3, P4) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL5(type, fn, num, P1, P2, P3, P4, P5) \
type fn(P1, P2, P3, P4, P5) \
{ \
	type a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
	return a; \
}

#define DEFN_SYSCALL0_VOID(fn, num) \
void fn() \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL1_VOID(fn, num, P1) \
void fn(P1) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL2_VOID(fn, num, P1, P2) \
void fn(P1, P2) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL3_VOID(fn, num, P1, P2, P3) \
void fn(P1, P2, P3) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL4_VOID(fn, num, P1, P2, P3, P4) \
void fn(P1, P2, P3, P4) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

#define DEFN_SYSCALL5_VOID(fn, num, P1, P2, P3, P4, P5) \
void fn(P1, P2, P3, P4, P5) \
{ \
	unsigned long a; \
	int reterrno; \
	asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	asm volatile("movl %%edx, %0" : "=r"(reterrno)); \
	if ( reterrno ) { errno = reterrno; } \
}

// Unknown platform with no implementation available.
#else
#error System call interface is not declared for host system.

#endif

__END_DECLS

#endif

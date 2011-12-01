/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	syscall.h
	Assembly stubs for declaring system calls in libmaxsi.

******************************************************************************/

#ifndef LIBMAXSI_SYSCALL_H
#define LIBMAXSI_SYSCALL_H

namespace Maxsi
{
#ifdef SORTIX_KERNEL
#warning ===
#warning Sortix does not support syscalls from within the kernel, building this part of LibMaxsi should not be needed, and should not be used in kernel mode
#warning ===
#endif

	#define DECL_SYSCALL0(type,fn) type fn();
	#define DECL_SYSCALL1(type,fn,p1) type fn(p1);
	#define DECL_SYSCALL2(type,fn,p1,p2) type fn(p1,p2);
	#define DECL_SYSCALL3(type,fn,p1,p2,p3) type fn(p1,p2,p3);
	#define DECL_SYSCALL4(type,fn,p1,p2,p3,p4) type fn(p1,p2,p3,p4);
	#define DECL_SYSCALL5(type,fn,p1,p2,p3,p4,p5) type fn(p1,p2,p3,p4,p5);

#ifdef PLATFORM_X86

	#define DEFN_SYSCALL0(type, fn, num) \
	inline type fn() \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL1(type, fn, num, P1) \
	inline type fn(P1 p1) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1)); \
		return a; \
	}

	#define DEFN_SYSCALL2(type, fn, num, P1, P2) \
	inline type fn(P1 p1, P2 p2) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2)); \
		return a; \
	}

	#define DEFN_SYSCALL3(type, fn, num, P1, P2, P3) \
	inline type fn(P1 p1, P2 p2, P3 p3) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2), "d" ((size_t)p3)); \
		return a; \
	}

	#define DEFN_SYSCALL4(type, fn, num, P1, P2, P3, P4) \
	inline type fn(P1 p1, P2 p2, P3 p3, P4 p4) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2), "d" ((size_t)p3), "D" ((size_t)p4)); \
	return a; \
	}

	#define DEFN_SYSCALL5(type, fn, num, P1, P2, P3, P4, P5) \
	inline type fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2), "d" ((size_t)p3), "D" ((size_t)p4), "S" ((size_t)p5)); \
		return a; \
	}

	#define DEFN_SYSCALL0_VOID(fn, num) \
	inline void fn() \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

	#define DEFN_SYSCALL1_VOID(fn, num, P1) \
	inline void fn(P1 p1) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1)); \
	}

	#define DEFN_SYSCALL2_VOID(fn, num, P1, P2) \
	inline void fn(P1 p1, P2 p2) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2)); \
	}

	#define DEFN_SYSCALL3_VOID(fn, num, P1, P2, P3) \
	inline void fn(P1 p1, P2 p2, P3 p3) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2), "d" ((size_t)p3)); \
	}

	#define DEFN_SYSCALL4_VOID(fn, num, P1, P2, P3, P4) \
	inline void fn(P1 p1, P2 p2, P3 p3, P4 p4) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2), "d" ((size_t)p3), "D" ((size_t)p4)); \
	}

	#define DEFN_SYSCALL5_VOID(fn, num, P1, P2, P3, P4, P5) \
	inline void fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num), "b" ((size_t)p1), "c" ((size_t)p2), "d" ((size_t)p3), "D" ((size_t)p4), "S" ((size_t)p5)); \
	}

#else

	#define DEFN_SYSCALL0(type, fn, num) \
	inline type fn() \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL1(type, fn, num, P1) \
	inline type fn(P1 p1) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL2(type, fn, num, P1, P2) \
	inline type fn(P1 p1, P2 p2) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL3(type, fn, num, P1, P2, P3) \
	inline type fn(P1 p1, P2 p2, P3 p3) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL4(type, fn, num, P1, P2, P3, P4) \
	inline type fn(P1 p1, P2 p2, P3 p3, P4 p4) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL5(type, fn, num, P1, P2, P3, P4, P5) \
	inline type fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
	{ \
		type a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
		return a; \
	}

	#define DEFN_SYSCALL0_VOID(fn, num) \
	inline void fn() \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

	#define DEFN_SYSCALL1_VOID(fn, num, P1) \
	inline void fn(P1 p1) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

	#define DEFN_SYSCALL2_VOID(fn, num, P1, P2) \
	inline void fn(P1 p1, P2 p2) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

	#define DEFN_SYSCALL3_VOID(fn, num, P1, P2, P3) \
	inline void fn(P1 p1, P2 p2, P3 p3) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

	#define DEFN_SYSCALL4_VOID(fn, num, P1, P2, P3, P4) \
	inline void fn(P1 p1, P2 p2, P3 p3, P4 p4) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

	#define DEFN_SYSCALL5_VOID(fn, num, P1, P2, P3, P4, P5) \
	inline void fn(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) \
	{ \
		size_t a; \
		asm volatile("int $0x80" : "=a" (a) : "0" (num)); \
	}

#endif

}

#endif


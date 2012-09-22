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

	bits.h
	Declares the required datatypes for the x86 architecture.

******************************************************************************/

#ifndef SORTIX_BITS_H
#define SORTIX_BITS_H

/* TODO: Declare stuff from <limits.h>. */

#undef __NULL
#if defined(__cplusplus)
	#define __NULL 0
#else
	#define __NULL ((void*) 0)
	typedef int __wchar_t;
#endif

/* Define basic signed types. */
typedef signed char __int8_t;
typedef signed short __int16_t;
typedef signed int __int32_t;
typedef signed long long __int64_t;

/* Define basic unsigned types. */
typedef unsigned char __uint8_t;
typedef unsigned short __uint16_t;
typedef unsigned int __uint32_t;
typedef unsigned long long __uint64_t;

typedef unsigned int __nat;

/* The maximum width integers available on this platform. */
typedef __int64_t __intmax_t;
typedef __uint64_t __uintmax_t;

/* Define an integer able to hold the size of the largest continuous memory */
/* region and define pointer safe integer types. */
//typedef __SIZE_TYPE__ __size_t;
typedef signed int __ssize_t;
typedef signed long __intptr_t;
typedef unsigned long __uintptr_t;
typedef signed long __ptrdiff_t;

#if defined(__x86_64__)
#define __WORDSIZE 64
#elif defined(__i386__)
#define __WORDSIZE 32
#else
#error __WORDSIZE not declared on this CPU.
#endif

# if __WORDSIZE == 64
#  define __INT64_C(c)	c ## L
#  define __UINT64_C(c)	c ## UL
# else
#  define __INT64_C(c)	c ## LL
#  define __UINT64_C(c)	c ## ULL
# endif

#define __INT8_MIN (127-1)
#define __INT16_MIN (-32767-1)
#define __INT32_MIN (-2147483647-1)
#define __INT64_MIN (-__INT64_C(9223372036854775807)-1)
#define __INT8_MAX (127)
#define __INT16_MAX (32767)
#define __INT32_MAX (2147483647)
#define __INT64_MAX (__INT64_C(9223372036854775807))
#define __UINT8_MAX (255)
#define __UINT16_MAX (65535)
#define __UINT32_MAX (4294967295U)
#define __UINT64_MAX (__UINT64_C(18446744073709551615))

#define __INTMAX_MIN __INT64_MIN
#define __INTMAX_MAX __INT64_MAX
#define __UINTMAX_MAX __UINT64_MAX

#define __CHAR_BIT (8)
#define __SCHAR_MIN __INT8_MIN
#define __SCHAR_MAX __INT8_MAX
#define __UCHAR_MAX __INT8_MAX
#define __CHAR_MIN __SCHAR_MIN
#define __CHAR_MAX __SCHAR_MAX

#define __WCHAR_MIN __INT_MIN
#define __WCHAR_MAX __INT_MAX

#define __SHRT_MIN __INT16_MIN
#define __SHRT_MAX __INT16_MAX
#define __USHRT_MAX __UINT16_MAX

#define __WORD_BIT (32)
#define __INT_MIN __INT32_MIN
#define __INT_MAX __INT32_MAX
#define __UINT_MAX __UINT32_MAX

#define __LONG_BIT (32)
#define __LONG_MIN __INT32_MIN
#define __LONG_MAX __INT32_MAX
#define __ULONG_MAX __UINT32_MAX

#define __LLONG_MIN __INT64_MIN
#define __LLONG_MAX __INT64_MAX
#define __ULLONG_MAX __UINT64_MAX

#define __SSIZE_MIN __INT_MIN
#define __SSIZE_MAX __INT_MAX
#define __SIZE_MAX __UINT_MAX
#define __INTPTR_MIN __INT_MIN
#define __INTPTR_MAX __INT_MAX
#define __UINTPTR_MAX __UINT_MAX

typedef int __wint_t;

typedef int __id_t;
typedef int __gid_t;
typedef int __pid_t;
typedef int __uid_t;
typedef int __locale_t; /* TODO: figure out what this does and typedef it properly. This is just a temporary assignment. */
typedef unsigned int __mode_t;
typedef __intmax_t __off_t;
typedef unsigned int __wctrans_t;  /* TODO: figure out what this does and typedef it properly. This is just a temporary assignment. */
typedef unsigned int __wctype_t;  /* TODO: figure out what this does and typedef it properly. This is just a temporary assignment. */
typedef unsigned int __useconds_t;
typedef __off_t __blksize_t;
typedef __off_t __blkcnt_t;
typedef unsigned int __nlink_t;
typedef __uintmax_t __ino_t;
typedef __uintptr_t __dev_t;
typedef __intmax_t __clock_t;
typedef long __time_t;
typedef long __suseconds_t;

#endif


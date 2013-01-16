/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2013.

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

    __/stdint.h
    Integer types.

*******************************************************************************/

#ifndef INCLUDE____STDINT_H
#define INCLUDE____STDINT_H

#define _LIBC_HACK_FEATURE_NO_DECLARATIONS
#include <features.h>
#include <__/wordsize.h>

__BEGIN_DECLS

/* TODO: Make the compiler provide all this information using __ prefix. */
/* TODO: Use __XINTXX_TYPE__ constants when switching to gcc 4.7.x or newer. */

/* Signed constants. */
#define __INT8_C(c) c
#define __INT16_C(c) c
#define __INT32_C(c) c
#if __WORDSIZE == 64
#define __INT64_C(c) c ## L
#else
#define __INT64_C(c) c ## LL
#endif

/* Unsigned constants. */
#define __UINT8_C(c) c
#define __UINT16_C(c) c
#define __UINT32_C(c) c ## U
#if __WORDSIZE == 64
#define __UINT64_C(c) c ## UL
#else
#define __UINT64_C(c) c ## ULL
#endif

/* Maxmimal constants. */
#if __WORDSIZE == 64
#define __INTMAX_C(c) c ## L
#define __UINTMAX_C(c) c ## UL
#else
#define __INTMAX_C(c) c ## LL
#define __UINTMAX_C(c) c ## ULL
#endif

/* Define basic signed types. */
typedef signed char __int8_t;
typedef short int __int16_t;
typedef int __int32_t;
#if __WORDSIZE == 64
typedef long int __int64_t;
#else
typedef long long int __int64_t;
#endif

#define __INT8_MIN (-128)
#define __INT16_MIN (-32767-1)
#define __INT32_MIN (-2147483647-1)
#define __INT64_MIN (-__INT64_C(9223372036854775807)-1)

#define __INT8_MAX (127)
#define __INT16_MAX (32767)
#define __INT32_MAX (2147483647)
#define __INT64_MAX (__INT64_C(9223372036854775807))

/* Define basic unsigned types. */
typedef unsigned char __uint8_t;
typedef unsigned short int __uint16_t;
typedef unsigned int __uint32_t;
#if __WORDSIZE == 64
typedef unsigned long int __uint64_t;
#else
typedef unsigned long long int __uint64_t;
#endif

#define __UINT8_MAX (255)
#define __UINT16_MAX (65535)
#define __UINT32_MAX (4294967295U)
#define __UINT64_MAX (__UINT64_C(18446744073709551615))

/* Define small signed types. */
typedef signed char __int_least8_t;
typedef short int __int_least16_t;
typedef int __int_least32_t;
#if __WORDSIZE == 64
typedef long int __int_least64_t;
#else
typedef long long int __int_least64_t;
#endif

#define __INT_LEAST8_MIN (-128)
#define __INT_LEAST16_MIN (-32767-1)
#define __INT_LEAST32_MIN (-2147483647-1)
#define __INT_LEAST64_MIN (-__INT64_C(9223372036854775807)-1)

#define __INT_LEAST8_MAX (127)
#define __INT_LEAST16_MAX (32767)
#define __INT_LEAST32_MAX (2147483647)
#define __INT_LEAST64_MAX (__INT64_C(9223372036854775807))

/* Define small unsigned types. */
typedef unsigned char __uint_least8_t;
typedef unsigned short int __uint_least16_t;
typedef unsigned int __uint_least32_t;
#if __WORDSIZE == 64
typedef unsigned long int __uint_least64_t;
#else
typedef unsigned long long int __uint_least64_t;
#endif

#define __UINT_LEAST8_MAX (255)
#define __UINT_LEAST16_MAX (65535)
#define __UINT_LEAST32_MAX (4294967295U)
#define __UINT_LEAST64_MAX (__UINT64_C(18446744073709551615))

/* Define fast signed types. */
typedef signed char __int_fast8_t;
#if __WORDSIZE == 64
typedef long int __int_fast16_t;
typedef long int __int_fast32_t;
typedef long int __int_fast64_t;
#else
typedef int __int_fast16_t;
typedef int __int_fast32_t;
typedef long long int __int_fast64_t;
#endif

#define __INT_FAST8_MIN (-128)
#if __WORDSIZE == 64
#define __INT_FAST16_MIN (-9223372036854775807L-1)
#define __INT_FAST32_MIN (-9223372036854775807L-1)
#else
#define __INT_FAST16_MIN (-2147483647-1)
#define __INT_FAST32_MIN (-2147483647-1)
#endif
#define __INT_FAST64_MIN (-__INT64_C(9223372036854775807)-1)

#define __INT_FAST8_MAX (127)
#if __WORDSIZE == 64
#define __INT_FAST16_MAX (9223372036854775807L)
#define __INT_FAST32_MAX (9223372036854775807L)
#else
#define __INT_FAST16_MAX (2147483647)
#define __INT_FAST32_MAX (2147483647)
#endif
#define __INT_FAST64_MAX (__INT64_C(9223372036854775807))

/* Define fast unsigned types. */
typedef unsigned char __uint_fast8_t;
#if __WORDSIZE == 64
typedef unsigned long int __uint_fast16_t;
typedef unsigned long int __uint_fast32_t;
typedef unsigned long int __uint_fast64_t;
#else
typedef unsigned int __uint_fast16_t;
typedef unsigned int __uint_fast32_t;
typedef unsigned long long int __uint_fast64_t;
#endif

#define __UINT_FAST8_MAX (255)
#if __WORDSIZE == 64
#define __UINT_FAST16_MAX (18446744073709551615UL)
#define __UINT_FAST32_MAX (18446744073709551615UL)
#else
#define __UINT_FAST16_MAX (4294967295U)
#define __UINT_FAST32_MAX (4294967295U)
#endif
#define __UINT_FAST64_MAX (__UINT64_C(18446744073709551615))

/* Define pointer-safe types. */
#if __WORDSIZE == 64
typedef long int __intptr_t;
typedef unsigned long int __uintptr_t;
#else
typedef int __intptr_t;
typedef unsigned int __uintptr_t;
#endif

#if __WORDSIZE == 64
#define __INTPTR_MIN (-9223372036854775807L-1)
#define __INTPTR_MAX (9223372036854775807L)
#define __UINTPTR_MAX (18446744073709551615UL)
#else
#define __INTPTR_MIN (-2147483647-1)
#define __INTPTR_MAX (2147483647)
#define __UINTPTR_MAX (4294967295U)
#endif

/* Define largest integer types. */
#if __WORDSIZE == 64
typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
#else
typedef long long int __intmax_t;
typedef long long unsigned int __uintmax_t;
#endif

#define __INTMAX_MIN (-__INT64_C(9223372036854775807)-1)
#define __INTMAX_MAX (__INT64_C(9223372036854775807))
#define __UINTMAX_MAX (__UINT64_C(18446744073709551615))

/* TODO: Should these come from a <__stddef.h>? */
#if __WORDSIZE == 64
#define __PTRDIFF_MIN (-9223372036854775807L-1)
#define __PTRDIFF_MAX (9223372036854775807L)
#else
#define __PTRDIFF_MIN (-2147483647-1)
#define __PTRDIFF_MAX (2147483647)
#endif

/* Note: The wchar_t related constants comes from another header. */

/* TODO: Should these come from a <__signal.h>? */
#define __SIG_ATOMIC_MIN (-2147483647-1)
#define __SIG_ATOMIC_MAX (2147483647)

/* TODO: Should these come from a <__stddef.h>? */
#if __WORDSIZE == 64
#define __SIZE_MAX (18446744073709551615UL)
#define __SSIZE_MIN (-9223372036854775807L-1)
#define __SSIZE_MAX (9223372036854775807L)
#else
#define __SIZE_MAX (4294967295U)
#define __SSIZE_MIN (-2147483647-1)
#define __SSIZE_MAX (2147483647)
#endif

__END_DECLS

#endif

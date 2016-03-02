/*
 * Copyright (c) 2011, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * stdint.h
 * Integer types.
 */

#ifndef INCLUDE_STDINT_H
#define INCLUDE_STDINT_H

#include <sys/cdefs.h>

#include <__/stdint.h>
#include <__/wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Only define these constant macros in C++ if requested. */
#if __USE_SORTIX || defined(__STDC_CONSTANT_MACROS) || !defined(__cplusplus)

#define INT8_C(c) __INT8_C(c)
#define INT16_C(c) __INT16_C(c)
#define INT32_C(c) __INT32_C(c)
#define INT64_C(c) __INT64_C(c)

#define UINT8_C(c) __UINT8_C(c)
#define UINT16_C(c) __UINT16_C(c)
#define UINT32_C(c) __UINT32_C(c)
#define UINT64_C(c) __UINT64_C(c)

#define INTMAX_C(c) __INTMAX_C(c)
#define UINTMAX_C(c) __UINTMAX_C(c)

#endif

/* Only define these limit macros in C++ if requested. */
#if __USE_SORTIX || defined(__STDC_LIMIT_MACROS) || !defined(__cplusplus)

#define INT8_MIN __INT8_MIN
#define INT16_MIN __INT16_MIN
#define INT32_MIN __INT32_MIN
#define INT64_MIN __INT64_MIN

#define INT8_MAX __INT8_MAX
#define INT16_MAX __INT16_MAX
#define INT32_MAX __INT32_MAX
#define INT64_MAX __INT64_MAX

#define UINT8_MAX __UINT8_MAX
#define UINT16_MAX __UINT16_MAX
#define UINT32_MAX __UINT32_MAX
#define UINT64_MAX __UINT64_MAX

#define INT_LEAST8_MIN __INT_LEAST8_MIN
#define INT_LEAST16_MIN __INT_LEAST16_MIN
#define INT_LEAST32_MIN __INT_LEAST32_MIN
#define INT_LEAST64_MIN __INT_LEAST64_MIN

#define INT_LEAST8_MAX __INT_LEAST8_MAX
#define INT_LEAST16_MAX __INT_LEAST16_MAX
#define INT_LEAST32_MAX __INT_LEAST32_MAX
#define INT_LEAST64_MAX __INT_LEAST64_MAX

#define UINT_LEAST8_MAX __UINT_LEAST8_MAX
#define UINT_LEAST16_MAX __UINT_LEAST16_MAX
#define UINT_LEAST32_MAX __UINT_LEAST32_MAX
#define UINT_LEAST64_MAX __UINT_LEAST64_MAX

#define INT_FAST8_MIN __INT_FAST8_MIN
#define INT_FAST16_MIN __INT_FAST16_MIN
#define INT_FAST32_MIN __INT_FAST32_MIN
#define INT_FAST64_MIN __INT_FAST64_MIN

#define INT_FAST8_MAX __INT_FAST8_MAX
#define INT_FAST16_MAX __INT_FAST16_MAX
#define INT_FAST32_MAX __INT_FAST32_MAX
#define INT_FAST64_MAX __INT_FAST64_MAX

#define UINT_FAST8_MAX __UINT_FAST8_MAX
#define UINT_FAST16_MAX __UINT_FAST16_MAX
#define UINT_FAST32_MAX __UINT_FAST32_MAX
#define UINT_FAST64_MAX __UINT_FAST64_MAX

#define INTPTR_MIN __INTPTR_MIN
#define INTPTR_MAX __INTPTR_MAX
#define UINTPTR_MAX __UINTPTR_MAX

#define INTMAX_MIN __INTMAX_MIN
#define INTMAX_MAX __INTMAX_MAX
#define UINTMAX_MAX __UINTMAX_MAX

#define PTRDIFF_MIN __PTRDIFF_MIN
#define PTRDIFF_MAX __PTRDIFF_MAX

#define SIG_ATOMIC_MIN __SIG_ATOMIC_MIN
#define SIG_ATOMIC_MAX __SIG_ATOMIC_MAX

#define SIZE_MAX __SIZE_MAX

#ifndef WCHAR_MIN
#define WCHAR_MIN __WCHAR_MIN
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX
#endif

#ifndef WINT_MIN
#define WINT_MIN __WINT_MIN
#endif

#ifndef WINT_MAX
#define WINT_MAX __WINT_MAX
#endif

#endif

typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;

typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;

typedef __int_least8_t int_least8_t;
typedef __int_least16_t int_least16_t;
typedef __int_least32_t int_least32_t;
typedef __int_least64_t int_least64_t;

typedef __uint_least8_t uint_least8_t;
typedef __uint_least16_t uint_least16_t;
typedef __uint_least32_t uint_least32_t;
typedef __uint_least64_t uint_least64_t;

typedef __int_fast8_t int_fast8_t;
typedef __int_fast16_t int_fast16_t;
typedef __int_fast32_t int_fast32_t;
typedef __int_fast64_t int_fast64_t;

typedef __uint_fast8_t uint_fast8_t;
typedef __uint_fast16_t uint_fast16_t;
typedef __uint_fast32_t uint_fast32_t;
typedef __uint_fast64_t uint_fast64_t;

typedef __intptr_t intptr_t;
typedef __uintptr_t uintptr_t;

typedef __intmax_t intmax_t;
typedef __uintmax_t uintmax_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

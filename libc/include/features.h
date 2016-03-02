/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * features.h
 * Detects the appropriate standard that this translation unit uses.
 */

#ifndef INCLUDE_FEATURES_H
#define INCLUDE_FEATURES_H

#define __sortix_libc__ 1

/* Detect whether we are a core system library. */
#if defined(__is_sortix_libc) || \
    defined(__is_sortix_libm)
#if !defined(__is_sortix_system_library)
#define __is_sortix_system_library
#endif
#endif

/* Detect whether we are a core system component. */
#if defined(__is_sortix_kernel) || defined(__is_sortix_system_library)
#if !defined(__is_sortix_system_component)
#define __is_sortix_system_component
#endif
#endif

/* Support macro to ease testing the compiler version. */
#define __GCC_PREREQ(gcc_major, gcc_minor) \
	(((gcc_major) == __GNUC__ && (gcc_minor) >= __GNUC_MINOR__) || \
	 ((gcc_major) < __GNUC__))

/* Sortix system components implicitly use the native API. */
#if defined(__is_sortix_system_component) && !defined(_SORTIX_SOURCE)
#define _SORTIX_SOURCE 1
#endif

/* These macros are used internally by the standard library to decide which
   symbols are provided by the standard library headers. They are always defined
   and have a value of zero if the feature is not activated. */
#undef __USE_C
#undef __USE_POSIX
#undef __USE_XOPEN
#undef __USE_SORTIX

/* Determine if all symbols should be visible in an effort to be as compatible
   as possible, even if the Sortix API doesn't have them in these places. */
#if defined(_ALL_SOURCE)
	#if !defined(_C11_SOURCE)
		#define _C11_SOURCE
	#endif
	#if !defined(_POSIX_C_SOURCE)
		#define _POSIX_C_SOURCE 200809L
	#endif
	#if !defined(_XOPEN_SOURCE)
		#define _XOPEN_SOURCE 700
	#endif
	#if !defined(_SORTIX_SOURCE)
		#define _SORTIX_SOURCE 1
		#define __IMPLICIT_SORTIX_SOURCE
	#endif
#endif

/* Default to the native API if no other base feature macros is defined. */
#if !defined(__STRICT_ANSI__) && \
    !defined(_ANSI_SOURCE) && \
    !defined(_ISOC99_SOURCE) && \
    !defined(_ISOC11_SOURCE) && \
    !defined(_XOPEN_SOURCE) && \
    !defined(_POSIX_SOURCE) && \
    !defined(_POSIX_C_SOURCE) && \
    !defined(_ALL_SOURCE) && \
    !defined(_SORTIX_SOURCE) && \
    !defined(_DEFAULT_SOURCE)
	#define _DEFAULT_SOURCE 1
#endif

/* Use the Sortix API if the default API was requested. */
#if defined(_DEFAULT_SOURCE) && !defined(_SORTIX_SOURCE)
	#define _SORTIX_SOURCE 1
	#define __IMPLICIT_SORTIX_SOURCE
#endif

/* Particular XSI revisions imply certain POSIX revisions. */
#if defined(_XOPEN_SOURCE)
	#if 700 <= _XOPEN_SOURCE - 0
		#undef _POSIX_C_SOURCE
		#define _POSIX_C_SOURCE 200809L
	#elif 600 <= _XOPEN_SOURCE - 0
		#undef _POSIX_C_SOURCE
		#define _POSIX_C_SOURCE 200112L
	#elif 520 <= _XOPEN_SOURCE - 0
		#undef _POSIX_C_SOURCE
		#define _POSIX_C_SOURCE 199506L
	#elif 500 <= _XOPEN_SOURCE - 0
		#undef _POSIX_C_SOURCE
		#define _POSIX_C_SOURCE 199506L
	#endif
#endif

/* C89 is always visible. */
#define __USE_C 1989

/* Determine if C90 Amendment 1:1995 is visible. */
#if defined(__STDC_VERSION__) && 199409L <= __STDC_VERSION__
	#undef __USE_C
	#define __USE_C 1995
#endif

/* Determine if C99 is visible. */
#if defined(_ISOC99_SOURCE) || \
    (defined(__STDC_VERSION__) && 199901L <= __STDC_VERSION__) || \
    (defined(__cplusplus) && 201103L <= __cplusplus)
	#undef __USE_C
	#define __USE_C 1999
#endif

/* Determine if C11 is visible. */
#if defined(_ISOC11_SOURCE) || \
    (defined(__STDC_VERSION__) && 201112L <= __STDC_VERSION__)
	#undef __USE_C
	#define __USE_C 2011
#endif

/* Determine which revision of XSI is used. */
#if defined(_XOPEN_SOURCE)
	#if 700 <= _XOPEN_SOURCE - 0
		#define __USE_XOPEN 700
	#elif 600 <= _XOPEN_SOURCE - 0
		#define __USE_XOPEN 600
	#elif 520 <= _XOPEN_SOURCE - 0
		#define __USE_XOPEN 520
	#elif 500 <= _XOPEN_SOURCE - 0
		#define __USE_XOPEN 500
	#elif 1 == _XOPEN_SOURCE_EXTENDED - 0
		#define __USE_XOPEN 420
	#elif 4 <= _XOPEN_VERSION - 0
		#define __USE_XOPEN 400
	#else
		#define __USE_XOPEN 300
	#endif
#else
	#define __USE_XOPEN 0
#endif

/* Determine which revision of POSIX is used. */
#ifdef _POSIX_C_SOURCE
	#if 200809L <= _POSIX_C_SOURCE - 0
		#define __USE_POSIX 200809L
	#elif 200112L <= _POSIX_C_SOURCE - 0
		#define __USE_POSIX 200112L
	#elif 199506L <= _POSIX_C_SOURCE - 0
		#define __USE_POSIX 199506L
	#elif 199309L <= _POSIX_C_SOURCE - 0
		#define __USE_POSIX 199309L
	#elif 2 <= _POSIX_C_SOURCE - 0
		#define __USE_POSIX 199209L
	#else
		#define __USE_POSIX 199009L
	#endif
#elif defined(_POSIX_SOURCE)
	#define __USE_POSIX 198808L
#else
	#define __USE_POSIX 0
#endif

/* Upgrade the visible C revision in accordance with the used POSIX revision. */
#if 200112L <= __USE_POSIX && __USE_C < 1999
	#undef __USE_C
	#define __USE_C 1999
#elif 0 < __USE_POSIX && __USE_C < 1990
	#undef __USE_C
	#define __USE_C 1990
#endif

/* Determine whether the Sortix API is visible. */
#if defined(_SORTIX_SOURCE)
#define __USE_SORTIX 1
#else
#define __USE_SORTIX 0
#endif

/* Whether to override some "standard" functions with Sortix alternatives. */
#if !defined(__SORTIX_STDLIB_REDIRECTS)
	#if __USE_SORTIX && !defined(__IMPLICIT_SORTIX_SOURCE)
		#define __SORTIX_STDLIB_REDIRECTS 1
	#else
		#define __SORTIX_STDLIB_REDIRECTS 0
	#endif
#endif

/* Provide the restrict keyword when building system components. */
#if defined(__is_sortix_system_component) && !defined(__want_restrict)
#define __want_restrict 1
#endif

/* Detect whether the restrict keyword is available. */
#if __STDC_VERSION__ >= 199901L
#define __HAS_RESTRICT 1
#endif

/* Provide the full <stdint.h> in all system components. */
#if defined(__is_sortix_system_component)
#undef __STDC_CONSTANT_MACROS
#undef __STDC_FORMAT_MACROS
#undef __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#endif

/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * __/limits.h
 * Implementation-defined constants.
 */

#ifndef INCLUDE____LIMITS_H
#define INCLUDE____LIMITS_H

/* char */
#define __CHAR_BIT __CHAR_BIT__
#define __SCHAR_MIN (-__SCHAR_MAX - 1)
#define __SCHAR_MAX __SCHAR_MAX__
#if __SCHAR_MAX__ == __INT_MAX__
	#define __UCHAR_MAX (__SCHAR_MAX * 2U + 1U)
#else
	#define __UCHAR_MAX (__SCHAR_MAX * 2 + 1)
#endif
#ifdef __CHAR_UNSIGNED__
	#if __SCHAR_MAX__ == __INT_MAX__
		#define __CHAR_MIN 0U
	#else
		#define __CHAR_MIN 0
	#endif
	#define __CHAR_MAX __UCHAR_MAX
#else
	#define __CHAR_MIN __SCHAR_MIN
	#define __CHAR_MAX __SCHAR_MAX
#endif

/* short */
#define __SHRT_MIN (-__SHRT_MAX - 1)
#define __SHRT_MAX __SHRT_MAX__
#if __SHRT_MAX__ == __INT_MAX__
	#define __USHRT_MAX (__SHRT_MAX * 2U + 1U)
#else
	#define __USHRT_MAX (__SHRT_MAX * 2 + 1)
#endif

/* int */
#define __INT_MIN (-__INT_MAX - 1)
#define __INT_MAX __INT_MAX__
#define __UINT_MAX (__INT_MAX * 2U + 1U)

/* long */
#define __LONG_MIN (-__LONG_MAX - 1L)
#define __LONG_MAX __LONG_MAX__
#define __ULONG_MAX (__LONG_MAX * 2UL + 1UL)

/* long long */
#define __LLONG_MIN (-__LLONG_MAX - 1LL)
#define __LLONG_MAX __LONG_LONG_MAX__
#define __ULLONG_MAX (__LLONG_MAX * 2ULL + 1ULL)

#endif

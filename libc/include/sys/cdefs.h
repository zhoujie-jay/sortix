/*
 * Copyright (c) 2011, 2012, 2013, 2015 Jonas 'Sortie' Termansen.
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
 * sys/cdefs.h
 * Declares internal macros for the C programming language.
 */

#ifndef INCLUDE_SYS_CDEFS_H
#define INCLUDE_SYS_CDEFS_H

#include <features.h>

/* Preprocessor trick to turn anything into a string. */
#define __STRINGIFY(x) #x

/* Issue warning when this is used, except in defines, where the warning is
   inserted whenever the macro is expanded. This can be used to deprecated
   macros - and it happens on preprocessor level - so it shouldn't change any
   semantics of any code that uses such a macro. The argument msg should be a
   string that contains the warning. */
#define __PRAGMA_WARNING(msg) _Pragma(__STRINGIFY(GCC warning msg))

/* Use the real restrict keyword if it is available. Not that this really
   matters as gcc uses __restrict and __restrict__ as aliases for restrict, but
   it will look nicer after preprocessing. */
#if __HAS_RESTRICT
#undef __restrict
#define __restrict restrict
#endif

/* Provide the restrict keyword if requested and unavailable. */
#if !__HAS_RESTRICT && __want_restrict
#define restrict __restrict
#undef __HAS_RESTRICT
#define __HAS_RESTRICT 2
#endif

/* Macro to declare a weak alias. */
#if defined(__is_sortix_libc)
#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((weak, alias(#old)))
#endif

#define __pure2 __attribute__((__const__))

#endif

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

    features.h
    Various things for various systems, programs, compabillity, and whatnot.

*******************************************************************************/

#ifndef INCLUDE_FEATURES_H
#define INCLUDE_FEATURES_H 1

#define __sortix_libc__ 1

/* Detect whether we are a core system library. */
#if __is_sortix_libc || __is_sortix_libm
#if !defined(__is_sortix_system_library)
#define __is_sortix_system_library 1
#endif
#endif

/* Detect whether we are a core system component. */
#if __is_sortix_kernel || __is_sortix_system_library
#if !defined(__is_sortix_system_component)
#define __is_sortix_system_component 1
#endif
#endif

/* Support macro to ease testing the compiler version. */
#define __GCC_PREREQ(gcc_major, gcc_minor) \
	(((gcc_major) == __GNUC__ && (gcc_minor) >= __GNUC_MINOR__) || \
	 ((gcc_major) < __GNUC__))

/* Preprocessor trick to turn anything into a string. */
#define __STRINGIFY(x) #x

/* Issue warning when this is used, except in defines, where the warning is
   inserted whenever the macro is expanded. This can be used to deprecated
   macros - and it happens on preprocessor level - so it shouldn't change any
   semantics of any code that uses such a macro. The argument msg should be a
   string that contains the warning. */
#define __PRAGMA_WARNING(msg) _Pragma(__STRINGIFY(GCC warning msg))

/* C++ needs to know that types and declarations are C, not C++. */
#ifdef __cplusplus
	#define __BEGIN_DECLS   extern "C" {
	#define __END_DECLS     }
#else
	#define __BEGIN_DECLS
	#define __END_DECLS
#endif

/* Sortix system components implicitly use the native API. */
#if __is_sortix_system_component
#define _SORTIX_SOURCE 1
#endif

/* By default, assume the source is compiled using the native API. */
#if !defined(_SORTIX_SOURCE) && \
    !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_BSD_SOURCE) && \
    !defined(_SVID_SOURCE) && \
    !defined(_XOPEN_SOURCE) && !defined(_XOPEN_SOURCE_EXTENDED) && \
    !defined(_GNU_SOURCE) && \
    1
	#define _SORTIX_SOURCE 1
#endif

/* Whether to override some "standard" functions with Sortix alternatives. */
#if !defined(__SORTIX_STDLIB_REDIRECTS)
	#if defined(_SORTIX_SOURCE)
		#define __SORTIX_STDLIB_REDIRECTS 1
	#else
		#define __SORTIX_STDLIB_REDIRECTS 0
	#endif
#endif

/* Provide the restrict keyword when building system components. */
#if __is_sortix_system_component && !defined(__want_restrict)
#define __want_restrict 1
#endif

/* Detect whether the restrict keyword is available. */
#if __STDC_VERSION__ >= 199901L
#define __HAS_RESTRICT 1
#endif

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

/* Provide the full <stdint.h> in all system components. */
#if __is_sortix_system_component
#undef __STDC_CONSTANT_MACROS
#undef __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#define __pure2 __attribute__((__const__))

/* Don't provide things from standard headers that is not implemented. */
/*#define __SORTIX_SHOW_UNIMPLEMENTED*/

#if !defined(_LIBC_HACK_FEATURE_NO_DECLARATIONS)
#include <sys/__/types.h>
#endif

#undef _LIBC_HACK_FEATURE_NO_DECLARATIONS

#endif

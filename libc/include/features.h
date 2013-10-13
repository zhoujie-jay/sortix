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
    Detects the appropriate standard that this translation unit uses.

*******************************************************************************/

#ifndef INCLUDE_FEATURES_H
#define INCLUDE_FEATURES_H

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

/* Provide the full <stdint.h> in all system components. */
#if __is_sortix_system_component
#undef __STDC_CONSTANT_MACROS
#undef __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#endif

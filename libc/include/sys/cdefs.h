/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2015.

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

    sys/cdefs.h
    Declares internal macros for the C programming language.

*******************************************************************************/

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

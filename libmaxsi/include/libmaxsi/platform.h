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

	platform.h
	Defines platform specific stuff.

******************************************************************************/

#ifndef LIBMAXSI_PLATFORM_H
#define LIBMAXSI_PLATFORM_H

	// Detect which platform we are compiling to and declare some useful macros.
	#ifdef PLATFORM_X86
		#define CPU X86
	#endif

	#ifdef PLATFORM_X64
		#define CPU X64
	#endif

	#if defined(PLATFORM_X86) || defined(PLATFORM_X64)
		#define PLATFORM_X86_FAMILY
		#define CPU_FAMILY X86_FAMILY
	#endif

	// Libmaxsi is also compatible as a C library, if enabled.
	#ifdef LIBMAXSI_LIBRARY
		#if defined(SORTIX_KERNEL) && !defined(LIBMAXSI_NO_LIBC)
			#define LIBMAXSI_NO_LIBC
		#endif

		#ifndef LIBMAXSI_NO_LIBC
			#define LIBMAXSI_LIBC
		#endif

		#ifdef LIBMAXSI_LIBC
			#define DUAL_FUNCTION(Type, CName, LibMaxsiName, Parameters) \
				Type LibMaxsiName Parameters __attribute__ ((weak, alias (#CName))); \
				extern "C" Type CName Parameters
		#else
			#define DUAL_FUNCTION(Type, CName, LibMaxsiName, Parameters) \
				Type LibMaxsiName Parameters
		#endif
	#endif

	#ifdef SORTIX_KERNEL
		#define ASSERT(invariant) \
			if ( unlikely(!(invariant)) ) \
			{ \
				Sortix::PanicF("Assertion failure: %s:%u: %s: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #invariant); \
				while ( true ) { } \
			}
	#else

		#define ASSERT(invariant)

	#endif

	#define STATIC_ASSERT(condition) static_assert(condition, #condition)

	// Define common datatypes.
	#include "types.h"

#endif

#ifdef SORTIX_KERNEL
#include <sortix/kernel/platform.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#endif


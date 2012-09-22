/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

*******************************************************************************/

#ifndef LIBMAXSI_PLATFORM_H
#define LIBMAXSI_PLATFORM_H

	#if !defined(PLATFORM_X64) && defined(__x86_64__)
		#define PLATFORM_X64
	#elif !defined(PLATFORM_X86) && defined(__i386__)
		#define PLATFORM_X86
	#endif

	// Detect which platform we are compiling to and declare some useful macros.
	#if !defined(CPU) && defined(PLATFORM_X86)
		#define CPU X86
	#endif

	#if !defined(CPU) && defined(PLATFORM_X64)
		#define CPU X64
	#endif

	#if !defined(CPU_FAMILY) && defined(PLATFORM_X86) || defined(PLATFORM_X64)
		#define PLATFORM_X86_FAMILY
		#define CPU_FAMILY X86_FAMILY
	#endif

	// Libmaxsi is also compatible as a C library, if enabled.
	#ifdef LIBMAXSI_LIBRARY
		#define DUAL_FUNCTION(Type, CName, LibMaxsiName, Parameters) \
			Type LibMaxsiName Parameters __attribute__ ((weak, alias (#CName))); \
			extern "C" Type CName Parameters
	#endif

	// Define common datatypes.
	#include "types.h"

#endif

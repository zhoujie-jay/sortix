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

	types.h
	Declares the required datatypes for the x86 architecture.

******************************************************************************/

#if !defined(PLATFORM_X86) && !defined(PLATFORM_X64)
	#if defined(__LP64__)
		#define PLATFORM_X64
	#else
		#define PLATFORM_X86
	#endif
#endif

#if defined(PLATFORM_X86)
#include "x86/bits.h"
#elif defined(PLATFORM_X64)
#include "x64/bits.h"
#else
#warning Unsupported platform
#endif


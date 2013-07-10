/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/bits.h
    Declares the current platform macro.

*******************************************************************************/

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

#if (defined(SORTIX_KERNEL) || defined(LIBC_LIBRARY)) && !defined(OFF_MIN)
#define OFF_MIN __OFF_MIN
#endif
#if (defined(SORTIX_KERNEL) || defined(LIBC_LIBRARY)) && !defined(OFF_MAX)
#define OFF_MAX __OFF_MAX
#endif

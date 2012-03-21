/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	platform.h
	Defines platform specific stuff.

******************************************************************************/

#ifndef SORTIX_PLATFORM_H
#define SORTIX_PLATFORM_H

	// Detect if we are using the GNU Compiler Collection
	#if defined(__GNUC__)
		#define COMPILER_GCC
	#endif

	#ifdef COMPILER_GCC
		#define SORTIX_NORETURN __attribute__((noreturn))
		#define SORTIX_MAYALIAS __attribute__((__may_alias__))
		#define SORTIX_PACKED __attribute__((packed))
		#define likely(x) __builtin_expect((x),1)
		#define unlikely(x) __builtin_expect((x),0)
	#else
		#define SORTIX_NORETURN
		#define SORTIX_MAYALIAS
		#define SORTIX_PACKED
		#define likely(x) (x)
		#define unlikely(x) (x)
	#endif


	#include <libmaxsi/platform.h>

	#ifdef JSSORTIX
	namespace Sortix
	{
		namespace JSSortix
		{
			inline void __attribute__((noreturn)) Exit()
			{
				/* send reset command to the keyboard controller */
				asm volatile("out %%al, %%dx" : : "a" (0xfe), "d" (0x64));
				while (1);
			}
		}
	}
	#endif

	#if !defined(PLATFORM_SERIAL) && defined(JSSORTIX)
		#define PLATFORM_SERIAL
	#endif

	#define USER

#endif


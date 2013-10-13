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

	panic.h
	Displays an error whenever something critical happens.

******************************************************************************/

#ifndef SORTIX_PANIC_H
#define SORTIX_PANIC_H

namespace Sortix
{
	// This function halts the kernel. If you wish to give an error message first,
	// then you ought to call Panic instead.
	extern "C" void SORTIX_NORETURN HaltKernel();

	extern "C" void SORTIX_NORETURN Panic(const char* Error);
	extern "C" void SORTIX_NORETURN PanicF(const char* Format, ...);
	extern "C" void WaitForInterrupt();
}



#endif

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    syscall.h
    Handles system calls from userspace.

*******************************************************************************/

#ifndef SORTIX_SYSCALL_H
#define SORTIX_SYSCALL_H

#include <sortix/syscallnum.h>
#include "cpu.h"

namespace Sortix
{
	class Thread;

	namespace Syscall
	{
		void Init();
		void Register(size_t index, void* funcptr);
	}
}

extern "C" void syscall_handler();

#endif

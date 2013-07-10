/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    memorymanagement.cpp
    Functions that allow modification of virtual memory.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/memorymanagement.h>
#include "syscall.h"

namespace Sortix {
namespace Memory {

static int sys_memstat(size_t* memused, size_t* memtotal)
{
	size_t used;
	size_t total;
	Statistics(&used, &total);
	// TODO: Check if legal user-space buffers!
	if ( memused )
		*memused = used;
	if ( memtotal )
		*memtotal = total;
	return 0;
}

void InitCPU(multiboot_info_t* bootinfo);

void Init(multiboot_info_t* bootinfo)
{
	InitCPU(bootinfo);

	Syscall::Register(SYSCALL_MEMSTAT, (void*) sys_memstat);
}

} // namespace Memory
} // namespace Sortix

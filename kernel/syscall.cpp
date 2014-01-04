/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    syscall.cpp
    Handles system calls from user-space.

*******************************************************************************/

#include <stddef.h>

#include <sortix/syscallnum.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>

namespace Sortix {
namespace Syscall {

extern "C"
{
	size_t SYSCALL_MAX;
	volatile void* syscall_list[SYSCALL_MAX_NUM];
}

static int sys_bad_syscall()
{
	// TODO: Send signal, set errno, or crash/abort process?
	Log::PrintF("I am the bad system call!\n");
	return -1;
}

void Init()
{
	SYSCALL_MAX = SYSCALL_MAX_NUM;
	for ( size_t i = 0; i < SYSCALL_MAX_NUM; i++ )
		syscall_list[i] = (void*) sys_bad_syscall;
}

void Register(size_t index, void* function)
{

	if ( SYSCALL_MAX_NUM <= index )
		PanicF("Attempted to register system call %p to index %zu, but "
		       "SYSCALL_MAX_NUM = %zu", function, index, SYSCALL_MAX_NUM);
	syscall_list[index] = function;
}

} // namespace Syscall
} // namespace Sortix

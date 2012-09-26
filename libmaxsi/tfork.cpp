/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	tfork.cpp
	Create a new task inheriting some properties from the current task.

*******************************************************************************/

#include <sys/syscall.h>
#include <unistd.h>

DEFN_SYSCALL2(pid_t, sys_tfork, SYSCALL_TFORK, int, tforkregs_t*);

extern "C" pid_t tfork(int flags, tforkregs_t* regs)
{
	return sys_tfork(flags, regs);
}

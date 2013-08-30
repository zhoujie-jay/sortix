/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    sys/resource/getpriority.cpp
    Get the scheduling priority of a process, process group, or user.

*******************************************************************************/

#include <sys/resource.h>
#include <sys/syscall.h>

DEFN_SYSCALL2(int, sys_getpriority, SYSCALL_GETPRIORITY, int, id_t);

extern "C" int getpriority(int which, id_t who)
{
	return sys_getpriority(which, who);
}

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

    sys/stat/futimens.cpp
    Change file last access and modification times.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/syscall.h>

// TODO: You cannot currently pass array types to the DEFN_SYSCALL* family.
DEFN_SYSCALL2(int, sys_futimens, SYSCALL_FUTIMENS, int, const struct timespec*);

extern "C" int futimens(int fd, const struct timespec times[2])
{
	return sys_futimens(fd, times);
}

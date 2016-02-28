/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    sys/socket/setsockopt.c
    Set socket options.

*******************************************************************************/

#include <sys/socket.h>
#include <sys/syscall.h>

DEFN_SYSCALL5(int, sys_setsockopt, SYSCALL_SETSOCKOPT, int, int, int, const void*, size_t);

int setsockopt(int fd, int level, int option_name, const void* option_value,
               socklen_t option_size)
{
	return sys_setsockopt(fd, level, option_name, option_value, option_size);
}

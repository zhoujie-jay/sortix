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

    sys/socket/getsockopt.cpp
    Get socket options.

*******************************************************************************/

#include <sys/socket.h>
#include <sys/syscall.h>

DEFN_SYSCALL5(int, sys_getsockopt, SYSCALL_GETSOCKOPT, int, int, int, const void*, size_t*);

extern "C"
int getsockopt(int fd, int level, int option_name, void* restrict option_value,
               socklen_t* restrict option_size)
{
	return sys_getsockopt(fd, level, option_name, option_value, option_size);
}

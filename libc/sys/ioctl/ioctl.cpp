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

    sys/ioctl/ioctl.cpp
    Miscellaneous device control interface.

*******************************************************************************/

#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <stdarg.h>

DEFN_SYSCALL3(int, sys_ioctl, SYSCALL_IOCTL, int, int, void*);

extern "C" int ioctl(int fd, int request, ...)
{
	va_list ap;
	va_start(ap, request);
	void* ptr = va_arg(ap, void*);
	va_end(ap);
	return sys_ioctl(fd, request, ptr);
}
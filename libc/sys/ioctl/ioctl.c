/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2016.

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

    sys/ioctl/ioctl.c
    Miscellaneous device control interface.

*******************************************************************************/

#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>

DEFN_SYSCALL3(int, sys_ioctl, SYSCALL_IOCTL, int, int, uintptr_t);

int ioctl(int fd, int request, ...)
{
	uintptr_t arg;
	va_list ap;
	va_start(ap, request);
	switch ( __IOCTL_TYPE(request) )
	{
	case __IOCTL_TYPE_VOID: arg = 0; break;
	case __IOCTL_TYPE_INT: arg = (uintptr_t) va_arg(ap, int); break;
	case __IOCTL_TYPE_LONG: arg = (uintptr_t) va_arg(ap, long); break;
	case __IOCTL_TYPE_PTR: arg = (uintptr_t) va_arg(ap, void*); break;
	default: return errno = EINVAL, -1;
	}
	va_end(ap);
	return sys_ioctl(fd, request, arg);
}

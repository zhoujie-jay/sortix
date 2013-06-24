/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    fcntl/openat.cpp
    Open a file relative to directory.

*******************************************************************************/

#include <sys/syscall.h>
#include <fcntl.h>
#include <stdarg.h>

DEFN_SYSCALL4(int, sys_openat, SYSCALL_OPENAT, int, const char*, int, mode_t);

extern "C" int openat(int dirfd, const char* path, int flags, ...)
{
	mode_t mode = 0;
	if ( flags & O_CREAT )
	{
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}
	return sys_openat(dirfd, path, flags, mode);
}

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

	lseek.cpp
	Sets the offset on a file descriptor.

*******************************************************************************/

#include <sys/syscall.h>
#include <unistd.h>

DEFN_SYSCALL3_VOID(SysSeek, SYSCALL_SEEK, int, off_t*, int);

extern "C" off_t lseek(int fd, off_t offset, int whence)
{
	SysSeek(fd, &offset, whence);
	return offset;
}
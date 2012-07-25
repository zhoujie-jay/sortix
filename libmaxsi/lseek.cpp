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

	lseek.cpp
	Sets the offset on a file descriptor.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>
#include <unistd.h>

namespace Maxsi {

DEFN_SYSCALL3_VOID(SysSeek, SYSCALL_SEEK, int, off_t*, int);

extern "C" off_t lseek(int fd, off_t offset, int whence)
{
	SysSeek(fd, &offset, whence);
	return offset;
}

} // namespace Maxsi

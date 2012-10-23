/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    chmod.cpp
    Changes the mode bits of a file.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

DEFN_SYSCALL2(int, sys_chmod, SYSCALL_CHMOD, const char*, mode_t);

extern "C" int chmod(const char* path, mode_t mode)
{
	return sys_chmod(path, mode);
}

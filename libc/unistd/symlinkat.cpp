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

    unistd/symlinkat.cpp
    Create a symbolic link.

*******************************************************************************/

#include <sys/syscall.h>

#include <fcntl.h>
#include <unistd.h>

DEFN_SYSCALL3(int, sys_symlinkat, SYSCALL_SYMLINKAT, const char*, int, const char*);

extern "C" int symlinkat(const char* oldpath, int newdirfd, const char* newpath)
{
	return sys_symlinkat(oldpath, newdirfd, newpath);
}

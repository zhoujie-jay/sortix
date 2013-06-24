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

    unistd/linkat.cpp
    Give a new name to a file.

*******************************************************************************/

#include <sys/syscall.h>

#include <fcntl.h>
#include <unistd.h>

DEFN_SYSCALL5(int, sys_linkat, SYSCALL_LINKAT, int, const char*, int, const char*, int);

extern "C" int linkat(int olddirfd, const char* oldpath,
                      int newdirfd, const char* newpath,
                      int flags)
{
	return sys_linkat(olddirfd, oldpath, newdirfd, newpath, flags);
}

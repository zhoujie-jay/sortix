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

    kernelinfo.cpp
    Queries information about the kernel.

*******************************************************************************/

#include <sys/kernelinfo.h>
#include <sys/syscall.h>

DEFN_SYSCALL3(ssize_t, sys_kernelinfo, SYSCALL_KERNELINFO, const char*, char*, size_t);

extern "C" ssize_t kernelinfo(const char* req, char* resp, size_t resplen)
{
	return sys_kernelinfo(req, resp, resplen);
}

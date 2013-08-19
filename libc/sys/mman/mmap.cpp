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

    sys/mman/mmap.cpp
    Maps a window of a file into memory.

*******************************************************************************/

#include <sys/mman.h>
#include <sys/syscall.h>

// TODO: We use a wrapper system call here because there are too many parameters
//       to the system call for some platforms. We should extend the system call
//       ABI so we can do system calls with huge parameter lists and huge return
//       values portably - then we'll make a new mmap system call that uses this
//       mechanism if needed.

struct mmap_request /* duplicated in sortix/mmap.cpp */
{
	void* addr;
	size_t size;
	int prot;
	int flags;
	int fd;
	off_t offset;
};

DEFN_SYSCALL1(void*, sys_mmap_wrapper, SYSCALL_MMAP_WRAPPER, struct mmap_request*);

static void* mmap_wrapper(struct mmap_request* request)
{
	return sys_mmap_wrapper(request);
}

extern "C"
void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset)
{
	struct mmap_request request = { addr, size, prot, flags, fd, offset };
	return mmap_wrapper(&request);
}

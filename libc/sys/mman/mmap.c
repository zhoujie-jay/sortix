/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * sys/mman/mmap.c
 * Maps a window of a file into memory.
 */

#include <sys/mman.h>
#include <sys/syscall.h>

// TODO: We use a wrapper system call here because there are too many parameters
//       to the system call for some platforms. We should extend the system call
//       ABI so we can do system calls with huge parameter lists and huge return
//       values portably - then we'll make a new mmap system call that uses this
//       mechanism if needed.

struct mmap_request /* duplicated in kernel/mmap.cpp */
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

void* mmap(void* addr, size_t size, int prot, int flags, int fd, off_t offset)
{
	struct mmap_request request = { addr, size, prot, flags, fd, offset };
	return mmap_wrapper(&request);
}

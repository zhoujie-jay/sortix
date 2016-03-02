/*
 * Copyright (c) 2011, 2012, 2013, 2015 Jonas 'Sortie' Termansen.
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
 * memorymanagement.cpp
 * Functions that allow modification of virtual memory.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/mman.h>
#include <sortix/seek.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/segment.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {

int sys_memstat(size_t* memused, size_t* memtotal)
{
	size_t used;
	size_t total;
	Memory::Statistics(&used, &total);
	if ( memused && !CopyToUser(memused, &used, sizeof(used)) )
		return -1;
	if ( memtotal && !CopyToUser(memtotal, &total, sizeof(total)) )
		return -1;
	return 0;
}

} // namespace Sortix

namespace Sortix {
namespace Memory {

void UnmapMemory(Process* process, uintptr_t addr, size_t size)
{
	// process->segment_write_lock is held.
	// process->segment_lock is held.
	assert(Page::IsAligned(addr));
	assert(Page::IsAligned(size));
	assert(process == CurrentProcess());

	if ( UINTPTR_MAX - addr < size )
		size = Page::AlignDown(UINTPTR_MAX - addr);
	if ( !size )
		return;

	struct segment unmap_segment;
	unmap_segment.addr = addr;
	unmap_segment.size = size;
	unmap_segment.prot = 0;
	while ( struct segment* conflict = FindOverlappingSegment(process,
	                                                          &unmap_segment) )
	{
		// Delete the segment if covered entirely by our request.
		if ( addr <= conflict->addr && conflict->addr + conflict->size <= addr + size )
		{
			uintptr_t conflict_offset = (uintptr_t) conflict - (uintptr_t) process->segments;
			size_t conflict_index = conflict_offset / sizeof(struct segment);
			Memory::UnmapRange(conflict->addr, conflict->size, PAGE_USAGE_USER_SPACE);
			Memory::Flush();
			if ( conflict_index + 1 == process->segments_used )
			{
				process->segments_used--;
				continue;
			}
			process->segments[conflict_index] = process->segments[--process->segments_used];
			qsort(process->segments, process->segments_used,
			      sizeof(struct segment), segmentcmp);
			continue;
		}

		// Delete the middle of the segment if covered there by our request.
		if ( conflict->addr < addr && addr + size - conflict->addr <= conflict->size )
		{
			Memory::UnmapRange(addr, size, PAGE_USAGE_USER_SPACE);
			Memory::Flush();
			struct segment right_segment;
			right_segment.addr = addr + size;
			right_segment.size = conflict->addr + conflict->size - (addr + size);
			right_segment.prot = conflict->prot;
			conflict->size = addr - conflict->addr;
			// TODO: This shouldn't really fail as we free memory above, but
			//       this code isn't really provably reliable.
			if ( !AddSegment(process, &right_segment) )
				PanicF("Unexpectedly unable to split memory mapped segment");
			continue;
		}

		// Delete the part of the segment covered partially from the left.
		if ( addr <= conflict->addr )
		{
			Memory::UnmapRange(conflict->addr, addr + size - conflict->addr, PAGE_USAGE_USER_SPACE);
			Memory::Flush();
			conflict->size = conflict->addr + conflict->size - (addr + size);
			conflict->addr = addr + size;
			continue;
		}

		// Delete the part of the segment covered partially from the right.
		if ( conflict->addr <= addr + size )
		{
			Memory::UnmapRange(addr, conflict->addr + conflict->size - addr, PAGE_USAGE_USER_SPACE);
			Memory::Flush();
			conflict->size -= conflict->addr + conflict->size - addr;
			continue;
		}
	}
}

bool ProtectMemory(Process* process, uintptr_t addr, size_t size, int prot)
{
	// process->segment_write_lock is held.
	// process->segment_lock is held.
	assert(Page::IsAligned(addr));
	assert(Page::IsAligned(size));
	assert(process == CurrentProcess());

	// First split the segments overlapping with [addr, addr + size) into
	// smaller segments that doesn't cross addr and addr+size, while verifying
	// there are no gaps in that region. This is where the operation can fail as
	// the AddSegment call can run out of memory. There is no harm in splitting
	// the segments into smaller chunks.
	for ( size_t offset = 0; offset < size; )
	{
		struct segment search_region;
		search_region.addr = addr + offset;
		search_region.size = Page::Size();
		search_region.prot = prot;
		struct segment* segment = FindOverlappingSegment(process, &search_region);

		if ( !segment )
			return errno = EINVAL, false;

		// Split the segment into two if it begins before our search region.
		if ( segment->addr < search_region.addr )
		{
			struct segment new_segment;
			new_segment.addr = search_region.addr;
			new_segment.size = segment->addr + segment->size - new_segment.addr;
			new_segment.prot = segment->prot;
			segment->size = search_region.addr - segment->addr;

			if ( !AddSegment(process, &new_segment) )
			{
				segment->size += new_segment.size;
				return false;
			}

			continue;
		}

		// Split the segment into two if it ends after addr + size.
		if ( size < segment->addr + segment->size - addr )
		{
			struct segment new_segment;
			new_segment.addr = addr + size;
			new_segment.size = segment->addr + segment->size - new_segment.addr;
			new_segment.prot = segment->prot;
			segment->size = addr + size - segment->addr;

			if ( !AddSegment(process, &new_segment) )
			{
				segment->size += new_segment.size;
				return false;
			}

			continue;
		}

		offset += segment->size;
	}

	// Run through all the segments in the region [addr, addr+size) and change
	// the permissions and update the permissions of the virtual memory itself.
	for ( size_t offset = 0; offset < size; )
	{
		struct segment search_region;
		search_region.addr = addr + offset;
		search_region.size = Page::Size();
		search_region.prot = prot;
		struct segment* segment = FindOverlappingSegment(process, &search_region);
		assert(segment);

		if ( segment->prot != prot )
		{
			// TODO: There is a moment of inconsistency here when the segment
			//       table itself has another protection written than what
			//       what applies to the actual pages.
			// TODO: SECURTIY: Does this have security implications?
			segment->prot = prot;
			for ( size_t i = 0; i < segment->size; i += Page::Size() )
				Memory::PageProtect(segment->addr + i, prot);
			Memory::Flush();
		}

		offset += segment->size;
	}

	return true;
}

bool MapMemory(Process* process, uintptr_t addr, size_t size, int prot)
{
	// process->segment_write_lock is held.
	// process->segment_lock is held.
	assert(Page::IsAligned(addr));
	assert(Page::IsAligned(size));
	assert(process == CurrentProcess());

	UnmapMemory(process, addr, size);

	struct segment new_segment;
	new_segment.addr = addr;
	new_segment.size = size;
	new_segment.prot = prot;

	if ( !MapRange(new_segment.addr, new_segment.size, new_segment.prot, PAGE_USAGE_USER_SPACE) )
		return false;
	Memory::Flush();

	if ( !AddSegment(process, &new_segment) )
	{
		UnmapRange(new_segment.addr, new_segment.size, PAGE_USAGE_USER_SPACE);
		Memory::Flush();
		return false;
	}

	// We have process->segment_write_lock locked, so we know that the memory in
	// user space exists and we can safely zero it here.
	// TODO: Another thread is able to see the old contents of the memory before
	//       we zero it causing potential information leaks.
	// TODO: SECURITY: Information leak.
	memset((void*) new_segment.addr, 0, new_segment.size);

	return true;
}

} // namespace Memory
} // namespace Sortix

namespace Sortix {

const int USER_SETTABLE_PROT = PROT_USER;
const int UNDERSTOOD_MMAP_FLAGS = MAP_SHARED |
                                  MAP_PRIVATE |
                                  MAP_ANONYMOUS |
                                  MAP_FIXED;

static
void* sys_mmap(void* addr_ptr, size_t size, int prot, int flags, int fd,
               off_t offset)
{
	// Verify that that the address is suitable aligned if fixed.
	uintptr_t addr = (uintptr_t) addr_ptr;
	if ( flags & MAP_FIXED && !Page::IsAligned(addr) )
		return errno = EINVAL, MAP_FAILED;
	// We don't allow zero-size mappings.
	if ( size == 0 )
		return errno = EINVAL, MAP_FAILED;
	// Verify that the user didn't request permissions not allowed.
	if ( prot & ~USER_SETTABLE_PROT )
		return errno = EINVAL, MAP_FAILED;
	// Verify that we understand all the flags we were passed.
	if ( flags & ~UNDERSTOOD_MMAP_FLAGS )
		return errno = EINVAL, MAP_FAILED;
	// Verify that MAP_PRIVATE and MAP_SHARED are not both set.
	if ( bool(flags & MAP_PRIVATE) == bool(flags & MAP_SHARED) )
		return errno = EINVAL, MAP_FAILED;
	// TODO: MAP_SHARED is not currently supported.
	if ( flags & MAP_SHARED )
		return errno = EINVAL, MAP_FAILED;
	// Verify the fíle descriptor and the offset is suitable set if needed.
	if ( !(flags & MAP_ANONYMOUS) &&
	     (fd < 0 || offset < 0 || (offset & (Page::Size()-1))) )
		return errno = EINVAL, MAP_FAILED;

	uintptr_t aligned_addr = Page::AlignDown(addr);
	uintptr_t aligned_size = Page::AlignUp(size);

	// Pick a good location near the end of user-space if no hint is given.
	if ( !(flags & MAP_FIXED) && !aligned_addr )
	{
		uintptr_t userspace_addr;
		size_t userspace_size;
		Memory::GetUserVirtualArea(&userspace_addr, &userspace_size);
		addr = aligned_addr =
			Page::AlignDown(userspace_addr + userspace_size - aligned_size);
	}

	// Verify that the offset + size doesn't overflow.
	if ( !(flags & MAP_ANONYMOUS) &&
	     (uintmax_t) (OFF_MAX - offset) < (uintmax_t) aligned_size )
		return errno = EOVERFLOW, MAP_FAILED;

	Process* process = CurrentProcess();

	// Verify whether the backing file is usable for memory mapping.
	ioctx_t ctx; SetupUserIOCtx(&ctx);
	Ref<Descriptor> desc;
	if ( !(flags & MAP_ANONYMOUS) )
	{
		if ( !(desc = process->GetDescriptor(fd)) )
			return MAP_FAILED;
		// Verify that the file is seekable.
		if ( desc->lseek(&ctx, 0, SEEK_CUR) < 0 )
			return errno = ENODEV, MAP_FAILED;
		// Verify that we have read access to the file.
		if ( desc->read(&ctx, NULL, 0) != 0 )
			return errno = EACCES, MAP_FAILED;
		// Verify that we have write access to the file if needed.
		if ( (prot & PROT_WRITE) && !(flags & MAP_PRIVATE) &&
		     desc->write(&ctx, NULL, 0) != 0 )
			return errno = EACCES, MAP_FAILED;
	}

	ScopedLock lock1(&process->segment_write_lock);
	ScopedLock lock2(&process->segment_lock);

	// Determine where to put the new segment and its protection.
	struct segment new_segment;
	if ( flags & MAP_FIXED )
		new_segment.addr = aligned_addr,
		new_segment.size = aligned_size;
	else if ( !PlaceSegment(&new_segment, process, (void*) addr, aligned_size, flags) )
		return errno = ENOMEM, MAP_FAILED;
	new_segment.prot = PROT_KWRITE | PROT_FORK;

	// Allocate a memory segment with the desired properties.
	if ( !Memory::MapMemory(process, new_segment.addr, new_segment.size, new_segment.prot) )
		return MAP_FAILED;

	// The pread will copy to user-space right requires this lock to be free.
	lock2.Reset();

	// Read the file contents into the newly allocated memory.
	if ( !(flags & MAP_ANONYMOUS) )
	{
		ioctx_t kctx; SetupKernelIOCtx(&kctx);
		for ( size_t so_far = 0; so_far < aligned_size; )
		{
			uint8_t* ptr = (uint8_t*) (new_segment.addr + so_far);
			size_t left = aligned_size - so_far;
			off_t pos = offset + so_far;
			ssize_t num_bytes = desc->pread(&kctx, ptr, left, pos);
			if ( num_bytes < 0 )
			{
				// TODO: How should this situation be handled? For now we'll
				//       just ignore the error condition.
				errno = 0;
				break;
			}
			if ( !num_bytes )
			{
				// We got an unexpected early end-of-file condition, but that's
				// alright as the MapMemory call zero'd the new memory and we
				// are expected to zero the remainder.
				break;
			}
			so_far += num_bytes;
		}
	}

	// Finally switch to the desired page protections.
	kthread_mutex_lock(&process->segment_lock);
	if ( prot & PROT_READ )
		prot |= PROT_KREAD;
	if ( prot & PROT_WRITE )
		prot |= PROT_KWRITE;
	prot |= PROT_FORK;
	Memory::ProtectMemory(CurrentProcess(), new_segment.addr, new_segment.size, prot);
	kthread_mutex_unlock(&process->segment_lock);

	lock1.Reset();

	return (void*) new_segment.addr;
}

int sys_mprotect(const void* addr_ptr, size_t size, int prot)
{
	// Verify that that the address is suitable aligned.
	uintptr_t addr = (uintptr_t) addr_ptr;
	if ( !Page::IsAligned(addr) )
		return errno = EINVAL, -1;
	// Verify that the user didn't request permissions not allowed.
	if ( prot & ~USER_SETTABLE_PROT )
		return errno = EINVAL, -1;

	size = Page::AlignUp(size);
	prot |= PROT_KREAD | PROT_KWRITE | PROT_FORK;

	Process* process = CurrentProcess();
	ScopedLock lock1(&process->segment_write_lock);
	ScopedLock lock2(&process->segment_lock);

	if ( !Memory::ProtectMemory(process, addr, size, prot) )
		return -1;

	return 0;
}

int sys_munmap(void* addr_ptr, size_t size)
{
	// Verify that that the address is suitable aligned.
	uintptr_t addr = (uintptr_t) addr_ptr;
	if ( !Page::IsAligned(addr) )
		return errno = EINVAL, -1;
	// We don't allow zero-size unmappings.
	if ( size == 0 )
		return errno = EINVAL, -1;

	size = Page::AlignUp(size);

	Process* process = CurrentProcess();
	ScopedLock lock1(&process->segment_write_lock);
	ScopedLock lock2(&process->segment_lock);

	Memory::UnmapMemory(process, addr, size);

	return 0;
}

// TODO: We use a wrapper system call here because there are too many parameters
//       to mmap for some platforms. We should extend the system call ABI so we
//       can do system calls with huge parameter lists and huge return values
//       portably - then we'll make sys_mmap use this mechanism if needed.

struct mmap_request /* duplicated in libc/sys/mman/mmap.cpp */
{
	void* addr;
	size_t size;
	int prot;
	int flags;
	int fd;
	off_t offset;
};

void* sys_mmap_wrapper(struct mmap_request* user_request)
{
	struct mmap_request request;
	if ( !CopyFromUser(&request, user_request, sizeof(request)) )
		return MAP_FAILED;
	return sys_mmap(request.addr, request.size, request.prot, request.flags,
	                request.fd, request.offset);
}

} // namespace Sortix

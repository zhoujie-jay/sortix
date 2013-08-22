/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    memorymanagement.cpp
    Functions that allow modification of virtual memory.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/mman.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/segment.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {
namespace Memory {

static int sys_memstat(size_t* memused, size_t* memtotal)
{
	size_t used;
	size_t total;
	Statistics(&used, &total);
	// TODO: Check if legal user-space buffers!
	if ( memused )
		*memused = used;
	if ( memtotal )
		*memtotal = total;
	return 0;
}

void UnmapMemory(Process* process, uintptr_t addr, size_t size)
{
	// process->segment_lock is held.
	assert(Page::IsAligned(addr));
	assert(Page::IsAligned(size));
	assert(process == CurrentProcess());

	struct segment unmap_segment;
	unmap_segment.addr = addr;
	unmap_segment.size = size;
	unmap_segment.prot = 0;
	while ( struct segment* conflict = FindOverlappingSegment(process,
	                                                          &unmap_segment) )
	{
		// Delete the segment if covered entirely by our request.
		if ( addr <= conflict->addr && conflict->addr + conflict->size - addr <= size )
		{
			uintptr_t conflict_offset = (uintptr_t) conflict - (uintptr_t) process->segments;
			size_t conflict_index = conflict_offset / sizeof(struct segment);
			Memory::UnmapRange(conflict->addr, conflict->size);
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
			Memory::UnmapRange(addr, size);
			Memory::Flush();
			struct segment right_segment;
			right_segment.addr = addr + size;
			right_segment.size = conflict->addr + conflict->size - (addr + size);
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
			Memory::UnmapRange(conflict->addr, addr + size - conflict->addr);
			Memory::Flush();
			conflict->size = conflict->addr + conflict->size - (addr + size);
			conflict->addr = addr + size;
			continue;
		}

		// Delete the part of the segment covered partially from the right.
		if ( conflict->addr + size <= addr + size )
		{
			Memory::UnmapRange(addr, addr + conflict->size + conflict->addr);
			Memory::Flush();
			conflict->size -= conflict->size + conflict->addr;
			continue;
		}
	}
}

bool ProtectMemory(Process* process, uintptr_t addr, size_t size, int prot)
{
	// process->segment_lock is held.
	assert(Page::IsAligned(addr));
	assert(Page::IsAligned(size));
	assert(process == CurrentProcess());

	// First split the segments overlapping with [addr, addr + size) into
	// smaller segments that doesn't cross addr and addr+size, while verifying
	// there are no gaps in that region. This is where the operation can fail as
	// the AddSegtment call can run out of memory. There is no harm in splitting
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
			segment->prot = prot;
			for ( size_t i = 0; i < segment->size; i += Page::Size() )
				Memory::PageProtect(segment->addr + i, prot);
		}

		offset += segment->size;
	}

	return true;
}

bool MapMemory(Process* process, uintptr_t addr, size_t size, int prot)
{
	// process->segment_lock is held.
	assert(Page::IsAligned(addr));
	assert(Page::IsAligned(size));
	assert(process == CurrentProcess());

	UnmapMemory(process, addr, size);

	struct segment new_segment;
	new_segment.addr = addr;
	new_segment.size = size;
	new_segment.prot = prot;

	if ( !MapRange(new_segment.addr, new_segment.size, new_segment.prot) )
		return false;
	Memory::Flush();

	if ( !AddSegment(process, &new_segment) )
	{
		UnmapRange(new_segment.addr, new_segment.size);
		Memory::Flush();
		return false;
	}

	// We have process->segment_lock locked, so we know that the memory in user
	// space exists and we can safely zero it here.
	// TODO: Another thread is able to see the old contents of the memory before
	//       we zero it causing potential information leaks.
	memset((void*) new_segment.addr, 0, new_segment.size);

	return true;
}

void InitCPU(multiboot_info_t* bootinfo);

void Init(multiboot_info_t* bootinfo)
{
	InitCPU(bootinfo);

	Syscall::Register(SYSCALL_MEMSTAT, (void*) sys_memstat);
}

} // namespace Memory
} // namespace Sortix

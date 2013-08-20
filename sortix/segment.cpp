/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    segment.cpp
    Structure representing a segment in a process.

*******************************************************************************/

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <sortix/kernel/decl.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/segment.h>

namespace Sortix {

bool AreSegmentsOverlapping(const struct segment* a, const struct segment* b)
{
	return a->addr < b->addr + b->size && b->addr < a->addr + a->size;
}

bool IsUserspaceSegment(const struct segment* segment)
{
	uintptr_t userspace_addr;
	size_t userspace_size;
	Memory::GetUserVirtualArea(&userspace_addr, &userspace_size);
	if ( segment->addr < userspace_addr )
		return false;
	uintptr_t userspace_end = userspace_addr + userspace_size;
	if ( userspace_end - segment->addr < segment->size )
		return false;
	return true;
}

struct segment* FindOverlappingSegment(Process* process, const struct segment* new_segment)
{
	// process->segment_lock is held at this point.

	// TODO: Speed up using binary search.
	for ( size_t i = 0; i < process->segments_used; i++ )
	{
		struct segment* segment = &process->segments[i];
		if ( AreSegmentsOverlapping(segment, new_segment) )
			return segment;
	}

	return NULL;
}

bool IsSegmentOverlapping(Process* process, const struct segment* new_segment)
{
	// process->segment_lock is held at this point.

	return FindOverlappingSegment(process, new_segment) != NULL;
}

bool AddSegment(Process* process, const struct segment* new_segment)
{
	// process->segment_lock is held at this point.

	// assert(!IsSegmentOverlapping(new_segment));

	// Check if we need to expand the segment list.
	if ( process->segments_used == process->segments_length )
	{
		size_t new_length = process->segments_length ?
		                    process->segments_length * 2 : 8;
		size_t new_size = new_length * sizeof(struct segment);
		struct segment* new_segments =
			(struct segment*) realloc(process->segments, new_size);
		if ( !new_segments )
			return false;
		process->segments = new_segments;
		process->segments_length = new_length;
	}

	// Add the new segment to the segment list.
	process->segments[process->segments_used++] = *new_segment;

	// Sort the segment list after address.
	qsort(process->segments, process->segments_used, sizeof(struct segment),
	      segmentcmp);

	return true;
}

} // namespace Sortix

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
 * segment.cpp
 * Structure representing a segment in a process.
 */

#include <sys/types.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <sortix/mman.h>

#include <sortix/kernel/decl.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/segment.h>
#include <sortix/kernel/yielder.h>

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

class segment_gaps
{
	typedef yielder_iterator<segment_gaps, struct segment> my_iterator;

public:
	segment_gaps(finished_yielder) : process(0) { }

	segment_gaps(Process* process) :
		process(process),
		current_segment_index(0),
		checked_leading(false),
		checked_trailing(false)
	{
		Memory::GetUserVirtualArea(&userspace_addr, &userspace_size);
	}

	bool yield(struct segment* result)
	{
		// process->segment_lock is held at this point.

		// Check if we have finished iterating all the segments.
		if ( !process )
			return false;

		// If the process has no segments at all, our job is really easy.
		if ( !process->segments_used )
		{
			result->addr = userspace_addr;
			result->size = userspace_size;
			result->prot = 0;
			process = NULL;
			return true;
		}

		// Find out whether there is a gap before the first segment.
		if ( !checked_leading && (checked_leading = true) &&
		     process->segments[0].addr != userspace_addr )
		{
			result->addr = userspace_addr;
			result->size = process->segments[0].addr - userspace_addr;
			result->prot = 0;
			return true;
		}

		// Search through the segments until a gap follows one.
		while ( current_segment_index + 1 < process->segments_used )
		{
			result->addr = process->segments[current_segment_index].addr +
			               process->segments[current_segment_index].size;
			result->size = process->segments[current_segment_index+1].addr -
			               result->addr;
			result->prot = 0;
			current_segment_index++;
			if ( result->size )
				return true;
		}

		// Find out if there is a gap after the last segment.
		if ( !checked_trailing && (checked_trailing = true) &&
		     process->segments[process->segments_used-1].addr +
		     process->segments[process->segments_used-1].size !=
		     userspace_addr + userspace_size )
		{
			result->addr = process->segments[process->segments_used-1].addr +
			               process->segments[process->segments_used-1].size;
			result->size = userspace_addr + userspace_size - result->addr;
			result->prot = 0;
			return true;
		}

		process = NULL;

		return false;
	}

	my_iterator begin() const
	{
		return my_iterator(segment_gaps(*this));
	}

	my_iterator end() const
	{
		return my_iterator(segment_gaps{finished_yielder{}});
	}

private:
	Process* process;
	uintptr_t userspace_addr;
	size_t userspace_size;
	size_t current_segment_index;
	bool checked_leading;
	bool checked_trailing;

};

bool PlaceSegment(struct segment* solution, Process* process, void* addr_ptr,
                  size_t size, int flags)
{
	// process->segment_lock is held at this point.

	assert(size);
	assert(!(flags & MAP_FIXED));

	uintptr_t addr = (uintptr_t) addr_ptr;
	size = Page::AlignUp(size);
	bool found_any = false;
	size_t best_distance = 0;
	struct segment best;

	for ( struct segment gap : segment_gaps(process) )
	{
		if ( gap.size < size )
			continue;
		if ( gap.addr <= addr && addr + size - gap.addr <= gap.size )
		{
			solution->addr = addr;
			solution->size = size;
			solution->prot = 0;
			return true;
		}
		struct segment attempt;
		size_t distance;
		attempt.addr = gap.addr;
		attempt.size = size;
		attempt.prot = 0;
		distance = addr < attempt.addr ? attempt.addr - addr : addr - attempt.addr;
		if ( !found_any|| distance < best_distance )
			found_any = true, best_distance = distance, best = attempt;
		attempt.addr = gap.addr + gap.size - size;
		attempt.size = size;
		attempt.prot = 0;
		distance = addr < attempt.addr ? attempt.addr - addr : addr - attempt.addr;
		if ( !found_any|| distance < best_distance )
			found_any = true, best_distance = distance, best = attempt;
	}

	return *solution = best, found_any;
}

} // namespace Sortix

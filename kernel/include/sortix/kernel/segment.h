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

    sortix/kernel/segment.h
    Structure representing a segment in a process.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_SEGMENT_H
#define INCLUDE_SORTIX_KERNEL_SEGMENT_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {

class Process;

struct segment
{
	uintptr_t addr;
	size_t size;
	int prot;
};

static inline int segmentcmp(const void* a_ptr, const void* b_ptr)
{
	const struct segment* a = (const struct segment*) a_ptr;
	const struct segment* b = (const struct segment*) b_ptr;
	return a->addr < b->addr ? -1 :
	       b->addr < a->addr ?  1 :
	       a->size < b->size ? -1 :
	       b->size < a->size ?  1 :
	                            0 ;
}

bool AreSegmentsOverlapping(const struct segment* a, const struct segment* b);
bool IsUserspaceSegment(const struct segment* segment);
struct segment* FindOverlappingSegment(Process* process, const struct segment* new_segment);
bool IsSegmentOverlapping(Process* process, const struct segment* new_segment);
bool AddSegment(Process* process, const struct segment* new_segment);
bool PlaceSegment(struct segment* solution, Process* process, void* addr_ptr,
                  size_t size, int flags);

} // namespace Sortix

#endif

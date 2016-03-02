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
 * sortix/kernel/segment.h
 * Structure representing a segment in a process.
 */

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

/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * linebuffer.cpp
 * Provides a simple queue-like line buffering for terminals.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/kernel/kernel.h>

#include "linebuffer.h"

namespace Sortix {

static size_t OffsetIndex(size_t offset, size_t index, size_t length)
{
	// TODO: Possible overflow here.
	return (offset + index) % length;
}

LineBuffer::LineBuffer()
{
	buffer = NULL;
	bufferlength = 0;
	bufferoffset = 0;
	buffercommitted = 0;
	bufferfrozen = 0;
	bufferused = 0;
}

LineBuffer::~LineBuffer()
{
	delete[] buffer;
}

bool LineBuffer::Push(uint32_t unicode)
{
	// Check if we need to allocate or resize the circular queue.
	if ( bufferused == bufferlength )
	{
		size_t newbufferlength = (bufferlength) ? bufferlength * 2 : 32UL;
		uint32_t* newbuffer = new uint32_t[newbufferlength];
		if ( !newbuffer )
			return false;
		size_t elemsize = sizeof(*buffer);
		size_t leadingavai = bufferlength-bufferoffset;
		size_t leading = (leadingavai < bufferused) ? leadingavai : bufferused;
		size_t trailing = bufferused - leading;
		if ( buffer )
		{
			memcpy(newbuffer, buffer + bufferoffset, leading * elemsize);
			memcpy(newbuffer + leading, buffer, trailing * elemsize);
			delete[] buffer;
		}
		buffer = newbuffer;
		bufferlength = newbufferlength;
		bufferoffset = 0;
	}

	size_t index = OffsetIndex(bufferoffset, bufferused++, bufferlength);
	buffer[index] = unicode;
	return true;
}

uint32_t LineBuffer::Pop()
{
	if ( !CanPop() )
		return 0;
	uint32_t result = Peek();
	bufferoffset = (bufferoffset+1) % bufferlength;
	buffercommitted--;
	bufferfrozen--;
	bufferused--;
	return result;
}

uint32_t LineBuffer::Peek() const
{
	if ( !CanPop() )
		return 0;
	size_t index = OffsetIndex(bufferoffset, 0, bufferlength);
	return buffer[index];
}

uint32_t LineBuffer::Backspace()
{
	if ( !CanBackspace() )
		return 0;
	size_t index = OffsetIndex(bufferoffset, --bufferused, bufferlength);
	return buffer[index];
}

uint32_t LineBuffer::WouldBackspace() const
{
	if ( !CanBackspace() )
		return 0;
	size_t index = OffsetIndex(bufferoffset, bufferused-1, bufferlength);
	return buffer[index];
}

void LineBuffer::Commit()
{
	buffercommitted = bufferfrozen = bufferused;
}

void LineBuffer::Freeze()
{
	bufferfrozen = bufferused;
}

bool LineBuffer::CanPop() const
{
	return buffercommitted;
}

bool LineBuffer::CanBackspace() const
{
	return bufferused - bufferfrozen;
}

void LineBuffer::Flush()
{
	bufferoffset = 0;
	buffercommitted = 0;
	bufferfrozen = 0;
	bufferused = 0;
}

} // namespace Sortix

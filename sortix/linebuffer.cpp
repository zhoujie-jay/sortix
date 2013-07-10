/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    linebuffer.cpp
    Provides a simple queue-like line buffering for terminals.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <string.h>
#include "linebuffer.h"

namespace Sortix
{
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
			if ( !newbuffer ) { return false; }
			size_t elemsize = sizeof(*buffer);
			size_t leadingavai = bufferlength-bufferoffset;
			size_t leading = (leadingavai < bufferused) ? leadingavai : bufferused;
			size_t trailing = bufferused - leading;
			memcpy(newbuffer, buffer + bufferoffset, leading * elemsize);
			memcpy(newbuffer + leading, buffer, trailing * elemsize);
			delete[] buffer;
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
		if ( !CanPop() ) { return 0; }
		uint32_t result = Peek();
		bufferoffset = (bufferoffset+1) % bufferlength;
		buffercommitted--;
		bufferfrozen--;
		bufferused--;
		return result;
	}

	uint32_t LineBuffer::Peek() const
	{
		if ( !CanPop() ) { return 0; }
		size_t index = OffsetIndex(bufferoffset, 0, bufferlength);
		return buffer[index];
	}

	uint32_t LineBuffer::Backspace()
	{
		if ( !CanBackspace() ) { return 0; }
		size_t index = OffsetIndex(bufferoffset, --bufferused, bufferlength);
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
}

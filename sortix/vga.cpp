/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	vga.h
	A Video Graphics Array driver.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include "vga.h"
#include "memorymanagement.h"
#include "scheduler.h"
#include "syscall.h"
#include "process.h"
#include "serialterminal.h"

using namespace Maxsi;

namespace Sortix
{
	namespace VGA
	{
		byte* const VGA = (byte* const) 0xB8000;
		const unsigned WIDTH = 80;
		const unsigned HEIGHT = 25;
		const size_t VGA_SIZE = sizeof(uint16_t) * WIDTH * HEIGHT;

		void Init()
		{
		}

		// Changes the position of the hardware cursor.
		void SetCursor(nat x, nat y)
		{
			nat value = x + y * WIDTH;

			// This sends a command to indicies 14 and 15 in the
			// CRT Control Register of the VGA controller. These
			// are the high and low bytes of the index that show
			// where the hardware cursor is to be 'blinking'.
			CPU::OutPortB(0x3D4, 14);
			CPU::OutPortB(0x3D5, (value >> 8) & 0xFF);
			CPU::OutPortB(0x3D4, 15);
			CPU::OutPortB(0x3D5, (value >> 0) & 0xFF);
		}		
	}

	DevVGA::DevVGA()
	{
		offset = 0;
	}

	DevVGA::~DevVGA()
	{
#ifdef PLATFORM_SERIAL
		// TODO: HACK: This is a hack that is unrelated to this file.
		// This is a hack to make the cursor a proper color after the vga buffer
		// has been radically modified. The best solution would be for the VGA
		// to ANSI Escape Codes converter to keep track of colors and restoring
		// them, but this will do for now.
		Log::PrintF("\e[m");
#endif
	}

	ssize_t DevVGA::Read(byte* dest, size_t count)
	{
		if ( VGA::VGA_SIZE - offset < count ) { count = VGA::VGA_SIZE - offset; }
		Maxsi::Memory::Copy(dest, VGA::VGA + offset, count);
		offset += count;
		return count;
	}

	ssize_t DevVGA::Write(const byte* src, size_t count)
	{
		if ( offset == VGA::VGA_SIZE && count ) { Error::Set(ENOSPC); return -1; }
		if ( VGA::VGA_SIZE - offset < count ) { count = VGA::VGA_SIZE - offset; }
		Maxsi::Memory::Copy(VGA::VGA + offset, src, count);
		offset = (offset + count) % VGA::VGA_SIZE;
		VGA::SetCursor(VGA::WIDTH, VGA::HEIGHT-1);
#ifdef PLATFORM_SERIAL
		SerialTerminal::OnVGAModified();
#endif
		return count;
	}

	bool DevVGA::IsReadable()
	{
		return true;
	}

	bool DevVGA::IsWritable()
	{
		return true;
	}

	size_t DevVGA::BlockSize()
	{
		return 1;
	}

	uintmax_t DevVGA::Size()
	{
		return VGA::VGA_SIZE;
	}

	uintmax_t DevVGA::Position()
	{
		return offset;
	}

	bool DevVGA::Seek(uintmax_t position)
	{
		if ( VGA::VGA_SIZE < position ) { Error::Set(EINVAL); return false; }
		offset = position;
		return true;
	}

	bool DevVGA::Resize(uintmax_t size)
	{
		if ( size == VGA::VGA_SIZE ) { return false; }
		Error::Set(ENOSPC);
		return false;
	}
}

/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	vga.cpp
	A Video Graphics Array driver.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/error.h>
#include <libmaxsi/memory.h>
#include "vga.h"
#include "scheduler.h"
#include "syscall.h"
#include "process.h"
#include "serialterminal.h"

#define TEST_VGAFONT 0

using namespace Maxsi;

namespace Sortix {
namespace VGA {

uint8_t* const VGA = (byte* const) 0xB8000;
const unsigned WIDTH = 80;
const unsigned HEIGHT = 25;
const size_t VGA_SIZE = sizeof(uint16_t) * WIDTH * HEIGHT;
size_t vgafontsize;
uint8_t* vgafont;

static void WriteIndex(uint16_t port, uint8_t index, uint8_t value)
{
	CPU::OutPortB(port, index);
	CPU::OutPortB(port+1, value);
}

static uint8_t ReadIndex(uint16_t port, uint8_t index)
{
	CPU::OutPortB(port, index);
	return CPU::InPortB(port+1);
}

static uint8_t ReplaceIndex(uint16_t port, uint8_t index, uint8_t value)
{
	uint8_t ret = ReadIndex(port, index);
	WriteIndex(port, index, value);
	return ret;
}

static void FetchVGAFont(uint8_t* font)
{
	// TODO: I just got these magic constants off the net.
	// Select plane 2 for reading.
	uint8_t old_03ce_04 = ReplaceIndex(0x03CE, 0x04, 0x02);
	// Clear even/odd mode.
	uint8_t old_03ce_05 = ReplaceIndex(0x03CE, 0x05, 0x04);
	// Map VGA Memory to 0xA0000 (Select 0xA0000-0xAFFFF).
	uint8_t old_03ce_06 = ReplaceIndex(0x03CE, 0x06, 0x04);
	// Set bitplane 2.
	uint8_t old_03c4_02 = ReplaceIndex(0x03C4, 0x02, 0x04);
	// Clear even/odd mode (the other way, don't ask why).
	uint8_t old_03c4_04 = ReplaceIndex(0x03C4, 0x04, 0x07);
	// The font data is now at 0xA0000, so fetch it. Note that there is
	// reserved room for a 8x32 resolution but we are using 8x16.
	const uint8_t* data = (const uint8_t*) 0xA0000UL;
	for ( size_t i = 0; i < VGA_FONT_NUMCHARS; i++ )
	{
		const uint8_t* src = data + (32*8)/8 * i;
		uint8_t* dest = font + VGA_FONT_CHARSIZE * i;
		Memory::Copy(dest, src, VGA_FONT_CHARSIZE);
	}
	// Restore the VGA state.
	WriteIndex(0x03C4, 0x02, old_03c4_02);
	WriteIndex(0x03C4, 0x04, old_03c4_04);
	WriteIndex(0x03CE, 0x05, old_03ce_05);
	WriteIndex(0x03CE, 0x06, old_03ce_06);
	WriteIndex(0x03CE, 0x04, old_03ce_04);
}

#if TEST_VGAFONT
static void PrintFontChar(const uint8_t* font, unsigned char c)
{
	const uint8_t* glyph = font + VGA_FONT_CHARSIZE * (size_t) c;
	for ( size_t y = 0; y < VGA_FONT_HEIGHT; y++ )
	{
		for ( size_t x = 0; x < VGA_FONT_WIDTH; x++ )
		{
			size_t bitindex = y * VGA_FONT_WIDTH + x;
			uint8_t bitmap = glyph[bitindex/8UL];
			uint8_t bitmod = bitindex % 8UL;
			uint8_t bitmask = 1U << bitmod;
			const char* toprint = (bitmap & bitmask) ? "X" : " ";
			Log::Print(toprint);
		}
		Log::Print("\n");
	}
}
#endif

const uint8_t* GetFont()
{
	return vgafont;
}

void Init()
{
	vgafontsize = VGA_FONT_NUMCHARS * VGA_FONT_CHARSIZE;
	if ( !(vgafont = new uint8_t[vgafontsize]) )
		Panic("Unable to allocate vga font buffer");
	FetchVGAFont(vgafont);
#if TEST_VGAFONT
	PrintFontChar(vgafont, 'A');
	PrintFontChar(vgafont, 'S');
#endif
}

// Changes the position of the hardware cursor.
void SetCursor(unsigned x, unsigned y)
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

} // namespace VGA

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

} // namespace Sortix

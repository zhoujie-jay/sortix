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

#include <errno.h>
#include <string.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/syscall.h>

#include "fs/util.h"
#include "vga.h"

#define TEST_VGAFONT 0

namespace Sortix {
namespace VGA {

uint8_t* const VGA = (uint8_t* const) 0xB8000;
const unsigned WIDTH = 80;
const unsigned HEIGHT = 25;
const size_t VGA_SIZE = sizeof(uint16_t) * WIDTH * HEIGHT;
size_t vgafontsize;
uint8_t* vgafont;

static void WriteIndex(uint16_t port, uint8_t index, uint8_t value)
{
	outport8(port, index);
	outport8(port+1, value);
}

static uint8_t ReadIndex(uint16_t port, uint8_t index)
{
	outport8(port, index);
	return inport8(port+1);
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
		memcpy(dest, src, VGA_FONT_CHARSIZE);
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

void Init(const char* devpath, Ref<Descriptor> slashdev)
{
	vgafontsize = VGA_FONT_NUMCHARS * VGA_FONT_CHARSIZE;
	if ( !(vgafont = new uint8_t[vgafontsize]) )
		Panic("Unable to allocate vga font buffer");
	FetchVGAFont(vgafont);
#if TEST_VGAFONT
	PrintFontChar(vgafont, 'A');
	PrintFontChar(vgafont, 'S');
#endif

	ioctx_t ctx; SetupKernelIOCtx(&ctx);

	// Setup the vgafont device.
	Ref<Inode> vgafontnode(new UtilMemoryBuffer(slashdev->dev, (ino_t) 0, 0, 0,
	                                            0660, vgafont, vgafontsize,
	                                            false, false));
	if ( !vgafontnode )
		PanicF("Unable to allocate %s/vgafont inode.", devpath);
	if ( LinkInodeInDir(&ctx, slashdev, "vgafont", vgafontnode) != 0 )
		PanicF("Unable to link %s/vgafont to vga font.", devpath);

	// Setup the vga device.
	Ref<Inode> vganode(new UtilMemoryBuffer(slashdev->dev, (ino_t) 0, 0, 0,
	                                        0660, VGA, VGA_SIZE, true, false));
	if ( !vganode )
		PanicF("Unable to allocate %s/vga inode.", devpath);
	if ( LinkInodeInDir(&ctx, slashdev, "vga", vganode) != 0 )
		PanicF("Unable to link %s/vga to vga.", devpath);
}

// Changes the position of the hardware cursor.
void SetCursor(unsigned x, unsigned y)
{
	unsigned value = x + y * WIDTH;

	// This sends a command to indicies 14 and 15 in the
	// CRT Control Register of the VGA controller. These
	// are the high and low bytes of the index that show
	// where the hardware cursor is to be 'blinking'.
	outport8(0x3D4, 14);
	outport8(0x3D5, (value >> 8) & 0xFF);
	outport8(0x3D4, 15);
	outport8(0x3D5, (value >> 0) & 0xFF);
}

} // namespace VGA
} // namespace Sortix

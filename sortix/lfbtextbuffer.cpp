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

    lfbtextbuffer.cpp
    An indexable text buffer rendered to a graphical linear frame buffer.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/vga.h>
#include <string.h>
#include "vga.h"
#include "lfbtextbuffer.h"

namespace Sortix {

static uint32_t ColorFromRGB(uint8_t r, uint8_t g, uint8_t b)
{
	union { struct { uint8_t b, g, r, a; }; uint32_t color; } ret;
	ret.r = r;
	ret.g = g;
	ret.b = b;
	ret.a = 255;
	return ret.color;
}

LFBTextBuffer* CreateLFBTextBuffer(uint8_t* lfb, uint32_t lfbformat,
                                   size_t xres, size_t yres, size_t scansize)
{
	size_t columns = xres / (VGA_FONT_WIDTH + 1);
	size_t rows = yres / VGA_FONT_HEIGHT;
	size_t fontsize = VGA_FONT_CHARSIZE * VGA_FONT_NUMCHARS;
	uint8_t* font;
	uint16_t* chars;
	uint16_t* attrs;
	LFBTextBuffer* ret;

	if ( !(font = new uint8_t[fontsize]) )
		goto cleanup_done;
	if ( !(chars = new uint16_t[columns * rows]) )
		goto cleanup_font;
	if ( !(attrs = new uint16_t[columns * rows]) )
		goto cleanup_chars;
	if ( !(ret = new LFBTextBuffer) )
		goto cleanup_attrs;

	memcpy(font, VGA::GetFont(), fontsize);
	ret->lfb = lfb;
	ret->lfbformat = lfbformat;
	ret->pixelsx = xres;
	ret->pixelsy = yres;
	ret->scansize = scansize;
	ret->columns = columns;
	ret->rows = rows;
	ret->font = font;
	memset(chars, 0, sizeof(uint16_t) * columns * rows);
	ret->chars = chars;
	memset(attrs, 0, sizeof(uint16_t) * columns * rows);
	ret->attrs = attrs;
	for ( size_t i = 0; i < 16UL; i++ )
	{
		uint8_t r = i & 0b0100 ? (i & 0b1000 ? 255 : 191) : 0;
		uint8_t g = i & 0b0010 ? (i & 0b1000 ? 255 : 191) : 0;
		uint8_t b = i & 0b0001 ? (i & 0b1000 ? 255 : 191) : 0;
		ret->colors[i] = ColorFromRGB(r, g, b);
	}
	ret->cursorenabled = true;
	ret->cursorpos = TextPos(0, 0);
	for ( size_t y = 0; y < yres; y++ )
		memset(lfb + scansize * y, 0, lfbformat/8UL * xres);
	return ret;

cleanup_attrs:
	delete[] attrs;
cleanup_chars:
	delete[] chars;
cleanup_font:
	delete[] font;
cleanup_done:
	return NULL;
}

// TODO: Those are also in vgatextbuffer.cpp. Move them to a shared location and
// give them better names.
static TextChar EntryToTextChar(uint16_t entry)
{
	char c = entry & 0x00FF;
	uint8_t vgacolor = entry >> 8U;
	return TextChar{c, vgacolor};
}

static uint16_t CharToTextEntry(TextChar c)
{
	unsigned char uc = c.c;
	return (uint16_t) uc | (uint16_t) c.vgacolor << 8U;
}

LFBTextBuffer::LFBTextBuffer()
{
}

LFBTextBuffer::~LFBTextBuffer()
{
	delete[] font;
	delete[] chars;
	delete[] attrs;
}

size_t LFBTextBuffer::Width() const
{
	return columns;
}

size_t LFBTextBuffer::Height() const
{
	return rows;
}

bool LFBTextBuffer::UsablePosition(TextPos pos) const
{
	return pos.x < columns && pos.y < rows;
}

TextPos LFBTextBuffer::CropPosition(TextPos pos) const
{
	if ( columns <= pos.x )
		pos.x = columns - 1;
	if ( rows <= pos.y )
		pos.y = rows -1;
	return pos;
}

TextPos LFBTextBuffer::AddToPosition(TextPos pos, size_t count)
{
	size_t index = pos.y * columns + pos.x + count;
	TextPos ret(index % columns, index / columns);
	return CropPosition(ret);
}

void LFBTextBuffer::RenderChar(uint16_t vgachar, size_t posx, size_t posy)
{
	if ( columns <= posx || rows <= posy )
		return;
	// TODO: Support other font sizes and resolutions.
	if ( lfbformat != 32 || VGA_FONT_WIDTH != 8UL )
		return;
	bool drawcursor = cursorenabled && posx == cursorpos.x && posy == cursorpos.y;
	uint8_t c = vgachar >> 0 & 0xFF;
	uint8_t fgcoloridx = vgachar >> 8 & 0x0F;
	uint8_t bgcoloridx = vgachar >> 12 & 0x0F;
	uint32_t fgcolor = colors[fgcoloridx];
	uint32_t bgcolor = colors[bgcoloridx];
	for ( size_t y = 0; y < VGA_FONT_HEIGHT; y++ )
	{
		size_t pixely = posy * VGA_FONT_HEIGHT + y;
		uint8_t linebitmap = font[c * VGA_FONT_CHARSIZE + y];
		for ( size_t x = 0; x < VGA_FONT_WIDTH+1; x++ )
		{
			uint32_t* line = (uint32_t*) (lfb + pixely * scansize);
			size_t pixelx = posx * (VGA_FONT_WIDTH+1) + x;
			bool fg = x != VGA_FONT_WIDTH && linebitmap & 1U << (7-x);
			line[pixelx] = fg ? fgcolor : bgcolor;
		}
	}
	if ( likely(!drawcursor) )
		return;
	for ( size_t y = VGA_FONT_HEIGHT - 2; y < VGA_FONT_HEIGHT; y++ )
	{
		size_t pixely = posy * VGA_FONT_HEIGHT + y;
		for ( size_t x = 0; x < VGA_FONT_WIDTH+1; x++ )
		{
			uint32_t* line = (uint32_t*) (lfb + pixely * scansize);
			size_t pixelx = posx * (VGA_FONT_WIDTH+1) + x;
			line[pixelx] = fgcolor;
		}
	}
}

void LFBTextBuffer::RenderCharAt(TextPos pos)
{
	RenderChar(chars[pos.y * columns + pos.x], pos.x, pos.y);
}

void LFBTextBuffer::RenderRegion(size_t c1, size_t r1, size_t c2, size_t r2)
{
	for ( size_t y = r1; y <= r2; y++ )
		for ( size_t x = c1; x <= c2; x++ )
			RenderChar(chars[y * columns + x], x, y);
}

void LFBTextBuffer::RenderRange(TextPos from, TextPos to)
{
	from = CropPosition(from);
	to = CropPosition(to);
	TextPos i = from;
	RenderChar(chars[i.y * columns + i.x], i.x, i.y);
	do
	{
		i = AddToPosition(i, 1);
		RenderChar(chars[i.y * columns + i.x], i.x, i.y);
	} while ( !(i.x==to.x && i.y==to.y) );
}

TextChar LFBTextBuffer::GetChar(TextPos pos) const
{
	if ( UsablePosition(pos) )
		return EntryToTextChar(chars[pos.y * columns + pos.x]);
	return {0, 0};
}

void LFBTextBuffer::SetChar(TextPos pos, TextChar c)
{
	if ( !UsablePosition(pos) )
		return;
	chars[pos.y * columns + pos.x] = CharToTextEntry(c);
	RenderRegion(pos.x, pos.y, pos.x, pos.y);
}

uint16_t LFBTextBuffer::GetCharAttr(TextPos pos) const
{
	if ( UsablePosition(pos) )
		return attrs[pos.y * columns + pos.x];
	return 0;
}

void LFBTextBuffer::SetCharAttr(TextPos pos, uint16_t attrval)
{
	if ( UsablePosition(pos) )
		attrs[pos.y * columns + pos.x] = attrval;
}

bool LFBTextBuffer::GetCursorEnabled() const
{
	return cursorenabled;
}

void LFBTextBuffer::SetCursorEnabled(bool enablecursor)
{
	bool updatecursor = cursorenabled != enablecursor;
	cursorenabled = enablecursor;
	if ( updatecursor )
		RenderCharAt(cursorpos);
}

TextPos LFBTextBuffer::GetCursorPos() const
{
	return cursorpos;
}

void LFBTextBuffer::SetCursorPos(TextPos newcursorpos)
{
	TextPos oldcursorpos = cursorpos;
	cursorpos = newcursorpos;
	RenderCharAt(oldcursorpos);
	RenderCharAt(newcursorpos);
}

size_t LFBTextBuffer::OffsetOfPos(TextPos pos) const
{
	return pos.y * columns + pos.x;
}

void LFBTextBuffer::Scroll(ssize_t off, TextChar fillwith)
{
	if ( !off )
		return;
	bool neg = 0 < off;
	size_t absoff = off < 0 ? -off : off;
	if ( rows < absoff )
		absoff = rows;
	TextPos scrollfrom = neg ? TextPos{0, absoff} : TextPos{0, 0};
	TextPos scrollto = neg ? TextPos{0, 0} : TextPos{0, absoff};
	TextPos fillfrom = neg ? TextPos{0, rows-absoff} : TextPos{0, 0};
	TextPos fillto = neg ? TextPos{columns-1, rows-1} : TextPos{columns-1, absoff-1};
	size_t scrollchars = columns * (rows-absoff);
	Move(scrollto, scrollfrom, scrollchars);
	Fill(fillfrom, fillto, fillwith, 0);
}

void LFBTextBuffer::Move(TextPos to, TextPos from, size_t numchars)
{
	to = CropPosition(to);
	from = CropPosition(from);
	size_t dest = OffsetOfPos(to);
	size_t src = OffsetOfPos(from);
	if ( dest < src )
		for ( size_t i = 0; i < numchars; i++ )
			chars[dest + i] = chars[src + i],
			attrs[dest + i] = attrs[src + i];
	else if ( src < dest )
		for ( size_t i = 0; i < numchars; i++ )
			chars[dest + numchars-1 - i] = chars[src + numchars-1 - i],
			attrs[dest + numchars-1 - i] = attrs[src + numchars-1 - i];
	TextPos toend = AddToPosition(to, numchars);
	RenderRange(to, toend);
}

void LFBTextBuffer::Fill(TextPos from, TextPos to, TextChar fillwith,
                         uint16_t fillattr)
{
	from = CropPosition(from);
	to = CropPosition(to);
	size_t start = OffsetOfPos(from);
	size_t end = OffsetOfPos(to);
	size_t entry = CharToTextEntry(fillwith);
	for ( size_t i = start; i <= end; i++ )
		chars[i] = entry,
		attrs[i] = fillattr;
	RenderRange(from, to);
}

} // namespace Sortix

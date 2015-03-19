/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

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

    vgatextbuffer.cpp
    An indexable text buffer with the VGA text mode framebuffer as backend.

*******************************************************************************/

#include <sortix/vga.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>

#include "vga.h"
#include "vgatextbuffer.h"

namespace Sortix {

VGATextBuffer::VGATextBuffer(uint16_t* vga, TextChar* chars, uint16_t* attr,
                             size_t width, size_t height)
{
	this->vga = vga;
	this->chars = chars;
	this->attr = attr;
	this->width = width;
	this->height = height;
	cursorpos = {0, 0};
	cursorenabled = true;
	UpdateCursor();
}

VGATextBuffer::~VGATextBuffer()
{
}

static uint16_t CharToTextEntry(TextChar c)
{
	int remap = VGA::MapWideToVGAFont(c.c);
	if ( remap < 0 )
		remap = 0 /* replacement character */;
	return (uint16_t) remap | (uint16_t) c.vgacolor << 8U;
}

bool VGATextBuffer::UsablePosition(TextPos pos) const
{
	return pos.x < width && pos.y < height;
}

TextPos VGATextBuffer::CropPosition(TextPos pos) const
{
	if ( width <= pos.x )
		pos.x = width - 1;
	if ( height <= pos.y )
		pos.y = height -1;
	return pos;
}

size_t VGATextBuffer::OffsetOfPos(TextPos pos) const
{
	return pos.y * width + pos.x;
}

size_t VGATextBuffer::Width() const
{
	return width;
}

size_t VGATextBuffer::Height() const
{
	return height;
}

TextChar VGATextBuffer::GetChar(TextPos pos) const
{
	if ( UsablePosition(pos) )
		return chars[OffsetOfPos(pos)];
	return {0, 0};
}

void VGATextBuffer::SetChar(TextPos pos, TextChar c)
{
	if ( UsablePosition(pos) )
		chars[OffsetOfPos(pos)] = c,
		vga[OffsetOfPos(pos)] = CharToTextEntry(c);
}

uint16_t VGATextBuffer::GetCharAttr(TextPos pos) const
{
	if ( UsablePosition(pos) )
		return attr[OffsetOfPos(pos)];
	return 0;
}

void VGATextBuffer::SetCharAttr(TextPos pos, uint16_t attrval)
{
	if ( UsablePosition(pos) )
		attr[OffsetOfPos(pos)] = attrval;
}

void VGATextBuffer::Scroll(ssize_t off, TextChar fillwith)
{
	if ( !off )
		return;
	bool neg = 0 < off;
	size_t absoff = off < 0 ? -off : off;
	if ( height < absoff )
		absoff = height;
	TextPos scrollfrom = neg ? TextPos{0, absoff} : TextPos{0, 0};
	TextPos scrollto = neg ? TextPos{0, 0} : TextPos{0, absoff};
	TextPos fillfrom = neg ? TextPos{0, height-absoff} : TextPos{0, 0};
	TextPos fillto = neg ? TextPos{width-1, height-1} : TextPos{width-1, absoff-1};
	size_t scrollchars = width * (height-absoff);
	Move(scrollto, scrollfrom, scrollchars);
	Fill(fillfrom, fillto, fillwith, 0);
}

void VGATextBuffer::Move(TextPos to, TextPos from, size_t numchars)
{
	size_t dest = OffsetOfPos(CropPosition(to));
	size_t src = OffsetOfPos(CropPosition(from));
	if ( dest < src )
		for ( size_t i = 0; i < numchars; i++ )
			chars[dest + i] = chars[src + i],
			vga[dest + i] = CharToTextEntry(chars[dest + i]),
			attr[dest + i] = attr[src + i];
	else if ( src < dest )
		for ( size_t i = 0; i < numchars; i++ )
			chars[dest + numchars-1 - i] = chars[src + numchars-1 - i],
			vga[dest + numchars-1 - i] = CharToTextEntry(chars[dest + numchars-1 - i]),
			attr[dest + numchars-1 - i] = attr[src + numchars-1 - i];
}

void VGATextBuffer::Fill(TextPos from, TextPos to, TextChar fillwith,
                         uint16_t fillattr)
{
	from = CropPosition(from);
	to = CropPosition(to);
	size_t start = OffsetOfPos(from);
	size_t end = OffsetOfPos(to);
	size_t entry = CharToTextEntry(fillwith);
	for ( size_t i = start; i <= end; i++ )
		chars[i] = fillwith,
		vga[i] = entry,
		attr[i] = fillattr;
}

bool VGATextBuffer::GetCursorEnabled() const
{
	return cursorenabled;
}

void VGATextBuffer::SetCursorEnabled(bool enablecursor)
{
	cursorenabled = enablecursor;
	UpdateCursor();
}

TextPos VGATextBuffer::GetCursorPos() const
{
	return cursorpos;
}

void VGATextBuffer::SetCursorPos(TextPos cursorpos)
{
	this->cursorpos = cursorpos;
	UpdateCursor();
}

void VGATextBuffer::UpdateCursor()
{
	if ( cursorenabled )
		VGA::SetCursor(cursorpos.x, cursorpos.y);
	else
		VGA::SetCursor(width, height-1);
}

void VGATextBuffer::SpawnThreads()
{
}

bool VGATextBuffer::EmergencyIsImpaired()
{
	return false;
}

bool VGATextBuffer::EmergencyRecoup()
{
	return true;
}

void VGATextBuffer::EmergencyReset()
{
	Fill(TextPos{0, 0}, TextPos{width-1, height-1}, TextChar{0, 0}, 0);
	SetCursorPos(TextPos{0, 0});
}

} // namespace Sortix

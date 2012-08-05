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

	lfbtextbuffer.h
	An indexable text buffer rendered to a graphical linear frame buffer.

*******************************************************************************/

#ifndef SORTIX_LFBTEXTBUFFER_H
#define SORTIX_LFBTEXTBUFFER_H

#include <sortix/kernel/textbuffer.h>

namespace Sortix {

// Needed information:
// Linear frame bufer
// - Pointer
// - Format (bit depth)
// - X resolution, Y resolution, scanline size
// Font
// - Pointer to our copy of the font.
// Color table for each VGA color in <sortix/vga.h>
// Number of columns and rows.
// Table of characters and attributes.

struct LFBTextBufferInfo
{
	uint8_t* lfb;
	size_t lfbformat;
	size_t xres;
	size_t yres;
	size_t columns;
	size_t rows;
	uint16_t* chars;
	uint16_t* attrs;
	uint8_t* vgafont;
};

class LFBTextBuffer : public TextBuffer
{
	friend LFBTextBuffer* CreateLFBTextBuffer(uint8_t* lfb, uint32_t lfbformat,
	                                          size_t xres, size_t yres,
	                                          size_t scansize);
private:
	LFBTextBuffer();

public:
	virtual ~LFBTextBuffer();
	virtual size_t Width() const;
	virtual size_t Height() const;
	virtual TextChar GetChar(TextPos pos) const;
	virtual void SetChar(TextPos pos, TextChar c);
	virtual uint16_t GetCharAttr(TextPos pos) const;
	virtual void SetCharAttr(TextPos pos, uint16_t attrval);
	virtual void Scroll(ssize_t off, TextChar fillwith);
	virtual void Move(TextPos to, TextPos from, size_t numchars);
	virtual void Fill(TextPos from, TextPos to, TextChar fillwith,
	                  uint16_t fillattr);
	virtual bool GetCursorEnabled() const;
	virtual void SetCursorEnabled(bool enablecursor);
	virtual TextPos GetCursorPos() const;
	virtual void SetCursorPos(TextPos newcursorpos);

private:
	void RenderChar(uint16_t vgachar, size_t posx, size_t posy);
	void RenderCharAt(TextPos pos);
	void RenderRegion(size_t c1, size_t r1, size_t c2, size_t r2);
	void RenderRange(TextPos from, TextPos to);
	bool UsablePosition(TextPos pos) const;
	size_t OffsetOfPos(TextPos pos) const;
	TextPos CropPosition(TextPos pos) const;
	TextPos AddToPosition(TextPos pos, size_t count);

private:
	uint8_t* lfb;
	uint8_t* font;
	uint16_t* chars;
	uint16_t* attrs;
	size_t columns;
	size_t rows;
	size_t pixelsx;
	size_t pixelsy;
	size_t scansize;
	uint32_t colors[16UL];
	uint32_t lfbformat;
	bool cursorenabled;
	TextPos cursorpos;

};

LFBTextBuffer* CreateLFBTextBuffer(uint8_t* lfb, uint32_t lfbformat,
                                   size_t xres, size_t yres, size_t scansize);

} // namespace Sortix

#endif

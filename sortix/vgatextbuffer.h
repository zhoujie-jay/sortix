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

    vgatextbuffer.h
    An indexable text buffer with the VGA text mode framebuffer as backend.

*******************************************************************************/

#ifndef SORTIX_VGATEXTBUFFER_H
#define SORTIX_VGATEXTBUFFER_H

#include <sortix/kernel/textbuffer.h>

namespace Sortix {

class VGATextBuffer : public TextBuffer
{
public:
	VGATextBuffer(uint16_t* vga, uint16_t* attr, size_t width, size_t height);
	virtual ~VGATextBuffer();
	virtual size_t Width() const;
	virtual size_t Height() const;
	virtual TextChar GetChar(TextPos pos) const;
	virtual void SetChar(TextPos pos, TextChar c);
	virtual uint16_t GetCharAttr(TextPos pos) const ;
	virtual void SetCharAttr(TextPos pos, uint16_t attrval);
	virtual void Scroll(ssize_t off, TextChar fillwith);
	virtual void Move(TextPos to, TextPos from, size_t numchars);
	virtual void Fill(TextPos from, TextPos to, TextChar fillwith,
	                  uint16_t fillattr);
	virtual bool GetCursorEnabled() const;
	virtual void SetCursorEnabled(bool enablecursor);
	virtual TextPos GetCursorPos() const;
	virtual void SetCursorPos(TextPos cursorpos);

private:
	bool UsablePosition(TextPos pos) const;
	TextPos CropPosition(TextPos pos) const;
	size_t OffsetOfPos(TextPos pos) const;
	void UpdateCursor();

private:
	uint16_t* vga;
	uint16_t* attr;
	size_t width;
	size_t height;
	TextPos cursorpos;
	bool cursorenabled;

};

} // namespace Sortix

#endif

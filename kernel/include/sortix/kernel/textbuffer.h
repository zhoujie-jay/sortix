/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014, 2015.

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

    sortix/kernel/textbuffer.h
    Provides a indexable text buffer for used by text mode terminals.

*******************************************************************************/

#ifndef SORTIX_TEXTBUFFER_H
#define SORTIX_TEXTBUFFER_H

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

struct TextPos
{
	TextPos() { }
	TextPos(size_t x, size_t y) : x(x), y(y) { }
	size_t x;
	size_t y;
};

struct TextCharPOD
{
	wchar_t c;
	uint8_t vgacolor; // Format of <sortix/vga.h>
};

struct TextChar
{
	TextChar() { }
	TextChar(const TextCharPOD& o) : c(o.c), vgacolor(o.vgacolor)  { }
	TextChar(wchar_t c, uint8_t vgacolor) : c(c), vgacolor(vgacolor) { }
	operator TextCharPOD() { return TextCharPOD{c, vgacolor}; }
	wchar_t c;
	uint8_t vgacolor; // Format of <sortix/vga.h>
};

static inline bool IsTextPosBeforeTextPos(const TextPos& a, const TextPos& b)
{
	return a.y < b.y || (a.y == b.y && a.x < b.x);
}

static inline bool IsTextPosAfterTextPos(const TextPos& a, const TextPos& b)
{
	return a.y > b.y || (a.y == b.y && a.x > b.x);
}

class TextBuffer
{
public:
	virtual ~TextBuffer() { }
	virtual size_t Width() const = 0;
	virtual size_t Height() const = 0;
	virtual TextChar GetChar(TextPos pos) const = 0;
	virtual void SetChar(TextPos pos, TextChar c) = 0;
	virtual uint16_t GetCharAttr(TextPos pos) const = 0;
	virtual void SetCharAttr(TextPos pos, uint16_t attr) = 0;
	virtual void Scroll(ssize_t off, TextChar fillwith) = 0;
	virtual void Move(TextPos to, TextPos from, size_t numchars) = 0;
	virtual void Fill(TextPos from, TextPos to, TextChar fillwith,
	                  uint16_t fillattr) = 0;
	virtual bool GetCursorEnabled() const = 0;
	virtual void SetCursorEnabled(bool enablecursor) = 0;
	virtual TextPos GetCursorPos() const = 0;
	virtual void SetCursorPos(TextPos cursorpos) = 0;
	virtual void SpawnThreads() = 0;
	virtual void Invalidate() = 0;
	virtual bool EmergencyIsImpaired() = 0;
	virtual bool EmergencyRecoup() = 0;
	virtual void EmergencyReset() = 0;

};

// The purpose of this handle class is such that the terminal driver can have
// its backing storage replaced at runtime, for instance if the user changes
// the screen resolution or the graphics driver. The backing text buffer can
// only be changed when there are no references (but our own) to the text buffer
// so don't forget to release it when you are done.
class TextBufferHandle
{
public:
	TextBufferHandle(TextBuffer* textbuf = NULL);
	~TextBufferHandle();
	TextBuffer* Acquire();
	void Release(TextBuffer* textbuf);
	void Replace(TextBuffer* newtextbuf);
	bool EmergencyIsImpaired();
	bool EmergencyRecoup();
	void EmergencyReset();
	TextBuffer* EmergencyAcquire();
	void EmergencyRelease(TextBuffer* textbuf);

private:
	kthread_mutex_t mutex;
	kthread_cond_t unusedcond;
	TextBuffer* textbuf;
	size_t numused;

};

} // namespace Sortix

#endif

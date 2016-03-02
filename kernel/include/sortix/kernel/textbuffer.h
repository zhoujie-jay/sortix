/*
 * Copyright (c) 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/textbuffer.h
 * Provides a indexable text buffer for used by text mode terminals.
 */

#ifndef SORTIX_TEXTBUFFER_H
#define SORTIX_TEXTBUFFER_H

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

static const uint16_t ATTR_CHAR = 1 << 0;
static const uint16_t ATTR_BOLD = 1 << 1;
static const uint16_t ATTR_UNDERLINE = 1 << 2;

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
	uint16_t attr;
};

struct TextChar
{
	TextChar() { }
	TextChar(const TextCharPOD& o) :
		c(o.c), vgacolor(o.vgacolor), attr(o.attr)  { }
	TextChar(wchar_t c, uint8_t vgacolor, uint16_t attr) :
		c(c), vgacolor(vgacolor), attr(attr) { }
	operator TextCharPOD() { return TextCharPOD{c, vgacolor, attr}; }
	wchar_t c;
	uint8_t vgacolor; // Format of <sortix/vga.h>
	uint16_t attr;
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
	virtual void Scroll(ssize_t off, TextChar fillwith) = 0;
	virtual void Move(TextPos to, TextPos from, size_t numchars) = 0;
	virtual void Fill(TextPos from, TextPos to, TextChar fillwith) = 0;
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

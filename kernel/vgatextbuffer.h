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
 * vgatextbuffer.h
 * An indexable text buffer with the VGA text mode framebuffer as backend.
 */

#ifndef SORTIX_VGATEXTBUFFER_H
#define SORTIX_VGATEXTBUFFER_H

#include <sortix/kernel/textbuffer.h>

namespace Sortix {

class VGATextBuffer : public TextBuffer
{
public:
	VGATextBuffer(uint16_t* vga, TextChar* chars, size_t width, size_t height);
	virtual ~VGATextBuffer();
	virtual size_t Width() const;
	virtual size_t Height() const;
	virtual TextChar GetChar(TextPos pos) const;
	virtual void SetChar(TextPos pos, TextChar c);
	virtual void Scroll(ssize_t off, TextChar fillwith);
	virtual void Move(TextPos to, TextPos from, size_t numchars);
	virtual void Fill(TextPos from, TextPos to, TextChar fillwith);
	virtual bool GetCursorEnabled() const;
	virtual void SetCursorEnabled(bool enablecursor);
	virtual TextPos GetCursorPos() const;
	virtual void SetCursorPos(TextPos cursorpos);
	virtual void SpawnThreads();
	virtual void Invalidate();
	virtual bool EmergencyIsImpaired();
	virtual bool EmergencyRecoup();
	virtual void EmergencyReset();

private:
	bool UsablePosition(TextPos pos) const;
	TextPos CropPosition(TextPos pos) const;
	size_t OffsetOfPos(TextPos pos) const;
	void UpdateCursor();

private:
	uint16_t* vga;
	TextChar* chars;
	size_t width;
	size_t height;
	TextPos cursorpos;
	bool cursorenabled;

};

} // namespace Sortix

#endif

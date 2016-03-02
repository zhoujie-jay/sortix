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
 * lfbtextbuffer.h
 * An indexable text buffer rendered to a graphical linear frame buffer.
 */

#ifndef SORTIX_LFBTEXTBUFFER_H
#define SORTIX_LFBTEXTBUFFER_H

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/textbuffer.h>

namespace Sortix {

enum TextBufferCmdType
{
	TEXTBUFCMD_EXIT = 0,
	TEXTBUFCMD_SYNC,
	TEXTBUFCMD_PAUSE,
	TEXTBUFCMD_CHAR,
	TEXTBUFCMD_CURSOR_SET_ENABLED,
	TEXTBUFCMD_CURSOR_MOVE,
	TEXTBUFCMD_MOVE,
	TEXTBUFCMD_FILL,
	TEXTBUFCMD_SCROLL,
	TEXTBUFCMD_REDRAW,
};

struct TextBufferCmd
{
	union { TextBufferCmdType type; size_t align; };
	union { size_t x; size_t from_x; ssize_t scroll_offset; };
	union { size_t y; size_t from_y; };
	union { size_t to_x; };
	union { size_t to_y; };
	union
	{
		bool b;
		TextCharPOD c;
		size_t val;
	};
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
	virtual void Scroll(ssize_t off, TextChar fillwith);
	virtual void Move(TextPos to, TextPos from, size_t numchars);
	virtual void Fill(TextPos from, TextPos to, TextChar fillwith);
	virtual bool GetCursorEnabled() const;
	virtual void SetCursorEnabled(bool enablecursor);
	virtual TextPos GetCursorPos() const;
	virtual void SetCursorPos(TextPos newcursorpos);
	virtual void SpawnThreads();
	virtual void Invalidate();
	virtual bool EmergencyIsImpaired();
	virtual bool EmergencyRecoup();
	virtual void EmergencyReset();

public:
	virtual void RenderThread();

private:
	void RenderChar(TextChar textchar, size_t posx, size_t posy);
	void RenderCharAt(TextPos pos);
	void RenderRegion(size_t c1, size_t r1, size_t c2, size_t r2);
	void RenderRange(TextPos from, TextPos to);
	bool UsablePosition(TextPos pos) const;
	size_t OffsetOfPos(TextPos pos) const;
	TextPos CropPosition(TextPos pos) const;
	TextPos AddToPosition(TextPos pos, size_t count);
	void DoScroll(ssize_t off, TextChar entry);
	void DoMove(TextPos to, TextPos from, size_t numchars);
	void DoFill(TextPos from, TextPos to, TextChar fillwith);
	void IssueCommand(TextBufferCmd* cmd);
	void StopRendering();
	void ResumeRendering();
	bool IsCommandIdempotent(const TextBufferCmd* cmd) const;
	void ExecuteCommand(TextBufferCmd* cmd,
	                    bool& exit_requested,
	                    bool& sync_requested,
	                    bool& pause_requested,
	                    TextPos& render_from,
	                    TextPos& render_to);

private:
	kthread_mutex_t execute_lock;
	kthread_mutex_t queue_lock;
	kthread_cond_t queue_not_full;
	kthread_cond_t queue_not_empty;
	kthread_cond_t queue_exit;
	kthread_cond_t queue_sync;
	kthread_cond_t queue_paused;
	kthread_cond_t queue_resume;
	TextBufferCmd* queue;
	size_t queue_length;
	size_t queue_offset;
	size_t queue_used;
	bool queue_is_paused;
	bool queue_thread;
	uint8_t* lfb;
	uint8_t* backbuf;
	uint8_t* font;
	TextChar* chars;
	size_t columns;
	size_t rows;
	size_t pixelsx;
	size_t pixelsy;
	size_t scansize;
	uint32_t colors[16UL];
	uint32_t lfbformat;
	bool cursorenabled;
	TextPos cursorpos;
	bool emergency_state;
	bool invalidated;
	size_t execute_amount;

};

LFBTextBuffer* CreateLFBTextBuffer(uint8_t* lfb, uint32_t lfbformat,
                                   size_t xres, size_t yres, size_t scansize);

} // namespace Sortix

#endif

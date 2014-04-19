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

    lfbtextbuffer.h
    An indexable text buffer rendered to a graphical linear frame buffer.

*******************************************************************************/

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
	TEXTBUFCMD_ATTR,
	TEXTBUFCMD_CURSOR_SET_ENABLED,
	TEXTBUFCMD_CURSOR_MOVE,
	TEXTBUFCMD_MOVE,
	TEXTBUFCMD_FILL,
	TEXTBUFCMD_SCROLL,
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
		struct { TextCharPOD c; uint16_t attr; };
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
	void DoFill(TextPos from, TextPos to, TextChar fillwith, uint16_t fillattr);
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
	bool emergency_state;
	size_t execute_amount;

};

LFBTextBuffer* CreateLFBTextBuffer(uint8_t* lfb, uint32_t lfbformat,
                                   size_t xres, size_t yres, size_t scansize);

} // namespace Sortix

#endif

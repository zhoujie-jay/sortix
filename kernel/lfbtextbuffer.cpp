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

    lfbtextbuffer.cpp
    An indexable text buffer rendered to a graphical linear frame buffer.

*******************************************************************************/

#include <string.h>

#include <sortix/vga.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/kernel/thread.h>

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

static void LFBTextBuffer__RenderThread(void* user)
{
	((LFBTextBuffer*) user)->RenderThread();
}

LFBTextBuffer* CreateLFBTextBuffer(uint8_t* lfb, uint32_t lfbformat,
                                   size_t xres, size_t yres, size_t scansize)
{
	const size_t QUEUE_LENGTH = 1024;
	size_t columns = xres / (VGA_FONT_WIDTH + 1);
	size_t rows = yres / VGA_FONT_HEIGHT;
	size_t fontsize = VGA_FONT_CHARSIZE * VGA_FONT_NUMCHARS;
	uint8_t* backbuf;
	uint8_t* font;
	TextChar* chars;
	uint16_t* attrs;
	TextBufferCmd* queue;
	LFBTextBuffer* ret;

	Process* kernel_process = Scheduler::GetKernelProcess();

	if ( !(backbuf = new uint8_t[yres * scansize]) )
		goto cleanup_done;
	if ( !(font = new uint8_t[fontsize]) )
		goto cleanup_backbuf;
	if ( !(chars = new TextChar[columns * rows]) )
		goto cleanup_font;
	if ( !(attrs = new uint16_t[columns * rows]) )
		goto cleanup_chars;
	if ( !(queue = new TextBufferCmd[QUEUE_LENGTH]) )
		goto cleanup_attrs;
	if ( !(ret = new LFBTextBuffer) )
		goto cleanup_queue;

	memcpy(font, VGA::GetFont(), fontsize);
	ret->execute_lock = KTHREAD_MUTEX_INITIALIZER;
	ret->queue_lock = KTHREAD_MUTEX_INITIALIZER;
	ret->queue_not_full = KTHREAD_COND_INITIALIZER;
	ret->queue_not_empty = KTHREAD_COND_INITIALIZER;
	ret->queue_exit = KTHREAD_COND_INITIALIZER;
	ret->queue_sync = KTHREAD_COND_INITIALIZER;
	ret->queue_paused = KTHREAD_COND_INITIALIZER;
	ret->queue_resume = KTHREAD_COND_INITIALIZER;
	ret->queue = queue;
	ret->queue_length = QUEUE_LENGTH;
	ret->queue_offset = 0;
	ret->queue_used = 0;
	ret->queue_is_paused = false;
	ret->queue_thread = false;
	ret->lfb = lfb;
	ret->backbuf = backbuf;
	ret->lfbformat = lfbformat;
	ret->pixelsx = xres;
	ret->pixelsy = yres;
	ret->scansize = scansize;
	ret->columns = columns;
	ret->rows = rows;
	ret->font = font;
	memset(chars, 0, sizeof(chars[0]) * columns * rows);
	ret->chars = chars;
	memset(attrs, 0, sizeof(attrs[0]) * columns * rows);
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
	ret->emergency_state = false;

	ret->queue_thread = true;
	if ( !RunKernelThread(kernel_process, LFBTextBuffer__RenderThread, ret) )
	{
		ret->queue_thread = false;
		delete ret;
		return NULL;
	}

	return ret;

cleanup_queue:
	delete[] queue;
cleanup_attrs:
	delete[] attrs;
cleanup_chars:
	delete[] chars;
cleanup_font:
	delete[] font;
cleanup_backbuf:
	delete[] backbuf;
cleanup_done:
	return NULL;
}

LFBTextBuffer::LFBTextBuffer()
{
}

LFBTextBuffer::~LFBTextBuffer()
{
	if ( queue_thread )
	{
		TextBufferCmd cmd;
		cmd.type = TEXTBUFCMD_EXIT;
		IssueCommand(&cmd);
		kthread_mutex_lock(&queue_lock);
		while ( queue_thread )
			kthread_cond_wait(&queue_exit, &queue_lock);
		kthread_mutex_unlock(&queue_lock);
	}
	delete[] backbuf;
	delete[] font;
	delete[] chars;
	delete[] attrs;
	delete[] queue;
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

void LFBTextBuffer::RenderChar(TextChar textchar, size_t posx, size_t posy)
{
	if ( columns <= posx || rows <= posy )
		return;
	// TODO: Support other font sizes and resolutions.
	if ( lfbformat != 32 || VGA_FONT_WIDTH != 8UL )
		return;
	bool drawcursor = cursorenabled && posx == cursorpos.x && posy == cursorpos.y;
	uint8_t fgcoloridx = textchar.vgacolor >> 0 & 0x0F;
	uint8_t bgcoloridx = textchar.vgacolor >> 4 & 0x0F;
	uint32_t fgcolor = colors[fgcoloridx];
	uint32_t bgcolor = colors[bgcoloridx];
	const uint8_t* charfont = VGA::GetCharacterFont(font, textchar.c);
	for ( size_t y = 0; y < VGA_FONT_HEIGHT; y++ )
	{
		size_t pixely = posy * VGA_FONT_HEIGHT + y;
		uint8_t linebitmap = charfont[y];
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
#if !defined(HAS_FAST_VIDEO_MEMORY)
	uint8_t* orig_lfb = lfb;
	bool backbuffered = from.y != to.y;
	if ( backbuffered )
	{
		lfb = backbuf;
		from.x = 0;
		to.x = columns - 1;
	}
#endif
	TextPos i = from;
	RenderChar(chars[i.y * columns + i.x], i.x, i.y);
	while ( !(i.x==to.x && i.y==to.y) )
	{
		i = AddToPosition(i, 1);
		RenderChar(chars[i.y * columns + i.x], i.x, i.y);
	}
#if !defined(HAS_FAST_VIDEO_MEMORY)
	if ( backbuffered )
	{
		lfb = orig_lfb;
		size_t font_height = 16;
		size_t font_width = 9;
		size_t scanline_start = from.y * font_height;
		size_t scanline_end = ((to.y+1) * font_height) - 1;
		size_t linesize = font_width * sizeof(uint32_t) * columns;
		for ( size_t sc = scanline_start; sc <= scanline_end; sc++ )
		{
			size_t offset = sc * scansize;
			memcpy(lfb + offset, backbuf + offset, linesize);
		}
	}
#endif
}

void LFBTextBuffer::IssueCommand(TextBufferCmd* cmd)
{
	if ( emergency_state )
	{
		bool exit_requested = false;
		bool sync_requested = false;
		bool pause_requested = false;
		TextPos render_from(columns - 1, rows - 1);
		TextPos render_to(0, 0);
		ExecuteCommand(cmd, exit_requested, sync_requested, pause_requested, render_from, render_to);
		if ( !IsTextPosBeforeTextPos(render_to, render_from) )
			RenderRange(render_from, render_to);
		return;
	}
	ScopedLock lock(&queue_lock);
	while ( queue_used == queue_length )
		kthread_cond_wait(&queue_not_full, &queue_lock);
	if ( !queue_used )
		kthread_cond_signal(&queue_not_empty);
	queue[(queue_offset + queue_used++) % queue_length] = *cmd;
}

void LFBTextBuffer::StopRendering()
{
	if ( emergency_state )
		return;
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_PAUSE;
	IssueCommand(&cmd);
	ScopedLock lock(&queue_lock);
	while ( !queue_is_paused )
		kthread_cond_wait(&queue_paused, &queue_lock);
}

void LFBTextBuffer::ResumeRendering()
{
	if ( emergency_state )
		return;
	ScopedLock lock(&queue_lock);
	queue_is_paused = false;
	kthread_cond_signal(&queue_resume);
}

TextChar LFBTextBuffer::GetChar(TextPos pos) const
{
	if ( UsablePosition(pos) )
	{
		((LFBTextBuffer*) this)->StopRendering();
		TextChar ret = chars[pos.y * columns + pos.x];
		((LFBTextBuffer*) this)->ResumeRendering();
		return ret;
	}
	return {0, 0};
}

void LFBTextBuffer::SetChar(TextPos pos, TextChar c)
{
	if ( !UsablePosition(pos) )
		return;
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_CHAR;
	cmd.x = pos.x;
	cmd.y = pos.y;
	cmd.c = c;
	IssueCommand(&cmd);
}

uint16_t LFBTextBuffer::GetCharAttr(TextPos pos) const
{
	if ( UsablePosition(pos) )
	{
		((LFBTextBuffer*) this)->StopRendering();
		uint16_t ret = attrs[pos.y * columns + pos.x];
		((LFBTextBuffer*) this)->ResumeRendering();
		return ret;
	}
	return 0;
}

void LFBTextBuffer::SetCharAttr(TextPos pos, uint16_t attrval)
{
	if ( !UsablePosition(pos) )
		return;
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_ATTR;
	cmd.x = pos.x;
	cmd.y = pos.y;
	cmd.attr = attrval;
	IssueCommand(&cmd);
}

bool LFBTextBuffer::GetCursorEnabled() const
{
	((LFBTextBuffer*) this)->StopRendering();
	bool ret = cursorenabled;
	((LFBTextBuffer*) this)->ResumeRendering();
	return ret;
}

void LFBTextBuffer::SetCursorEnabled(bool enablecursor)
{
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_CURSOR_SET_ENABLED;
	cmd.b = enablecursor;
	IssueCommand(&cmd);
}

TextPos LFBTextBuffer::GetCursorPos() const
{
	((LFBTextBuffer*) this)->StopRendering();
	TextPos ret = cursorpos;
	((LFBTextBuffer*) this)->ResumeRendering();
	return ret;
}

void LFBTextBuffer::SetCursorPos(TextPos newcursorpos)
{
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_CURSOR_MOVE;
	cmd.x = newcursorpos.x;
	cmd.y = newcursorpos.y;
	IssueCommand(&cmd);
}

size_t LFBTextBuffer::OffsetOfPos(TextPos pos) const
{
	return pos.y * columns + pos.x;
}

void LFBTextBuffer::Scroll(ssize_t off, TextChar fillwith)
{
	if ( !off )
		return;
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_SCROLL;
	cmd.scroll_offset = off;
	cmd.c = fillwith;
	IssueCommand(&cmd);
}

void LFBTextBuffer::Move(TextPos to, TextPos from, size_t numchars)
{
	to = CropPosition(to);
	from = CropPosition(from);
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_MOVE;
	cmd.to_x = to.x;
	cmd.to_y = to.y;
	cmd.from_x = from.x;
	cmd.from_y = from.y;
	cmd.val = numchars;
	IssueCommand(&cmd);
}

void LFBTextBuffer::Fill(TextPos from, TextPos to, TextChar fillwith,
                         uint16_t fillattr)
{
	from = CropPosition(from);
	to = CropPosition(to);
	TextBufferCmd cmd;
	cmd.type = TEXTBUFCMD_FILL;
	cmd.from_x = from.x;
	cmd.from_y = from.y;
	cmd.to_x = to.x;
	cmd.to_y = to.y;
	cmd.c = fillwith;
	cmd.attr = fillattr;
	IssueCommand(&cmd);
}

void LFBTextBuffer::DoScroll(ssize_t off, TextChar entry)
{
	bool neg = 0 < off;
	size_t absoff = off < 0 ? -off : off;
	if ( rows < absoff )
		absoff = rows;
	TextPos scrollfrom = neg ? TextPos{0, absoff} : TextPos{0, 0};
	TextPos scrollto = neg ? TextPos{0, 0} : TextPos{0, absoff};
	TextPos fillfrom = neg ? TextPos{0, rows-absoff} : TextPos{0, 0};
	TextPos fillto = neg ? TextPos{columns-1, rows-1} : TextPos{columns-1, absoff-1};
	size_t scrollchars = columns * (rows-absoff);
	DoMove(scrollto, scrollfrom, scrollchars);
	DoFill(fillfrom, fillto, entry, 0);
}

void LFBTextBuffer::DoMove(TextPos to, TextPos from, size_t numchars)
{
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
}

void LFBTextBuffer::DoFill(TextPos from, TextPos to, TextChar fillwith,
                           uint16_t fillattr)
{
	size_t start = OffsetOfPos(from);
	size_t end = OffsetOfPos(to);
	for ( size_t i = start; i <= end; i++ )
		chars[i] = fillwith,
		attrs[i] = fillattr;
}

bool LFBTextBuffer::IsCommandIdempotent(const TextBufferCmd* cmd) const
{
	switch ( cmd->type )
	{
		case TEXTBUFCMD_EXIT: return true;
		case TEXTBUFCMD_SYNC: return true;
		case TEXTBUFCMD_PAUSE: return true;
		case TEXTBUFCMD_CHAR: return true;
		case TEXTBUFCMD_ATTR: return true;
		case TEXTBUFCMD_CURSOR_SET_ENABLED: return true;
		case TEXTBUFCMD_CURSOR_MOVE: return true;
		case TEXTBUFCMD_MOVE: return false;
		case TEXTBUFCMD_FILL: return true;
		case TEXTBUFCMD_SCROLL: return false;
		default: return false;
	}
}

void LFBTextBuffer::ExecuteCommand(TextBufferCmd* cmd,
                                   bool& exit_requested,
                                   bool& sync_requested,
                                   bool& pause_requested,
                                   TextPos& render_from,
                                   TextPos& render_to)
{
	switch ( cmd->type )
	{
		case TEXTBUFCMD_EXIT:
			exit_requested = true;
			break;
		case TEXTBUFCMD_SYNC:
			sync_requested = true;
			break;
		case TEXTBUFCMD_PAUSE:
			pause_requested = true;
			break;
		case TEXTBUFCMD_CHAR:
		{
			TextPos pos(cmd->x, cmd->y);
			chars[pos.y * columns + pos.x] = cmd->c;
			if ( IsTextPosBeforeTextPos(pos, render_from) )
				render_from = pos;
			if ( IsTextPosAfterTextPos(pos, render_to) )
				render_to = pos;
		} break;
		case TEXTBUFCMD_ATTR:
		{
			TextPos pos(cmd->x, cmd->y);
			attrs[pos.y * columns + pos.x] = cmd->attr;
		} break;
		case TEXTBUFCMD_CURSOR_SET_ENABLED:
			if ( cmd->b != cursorenabled )
			{
				cursorenabled = cmd->b;
				if ( IsTextPosBeforeTextPos(cursorpos, render_from) )
					render_from = cursorpos;
				if ( IsTextPosAfterTextPos(cursorpos, render_to) )
					render_to = cursorpos;
			}
			break;
		case TEXTBUFCMD_CURSOR_MOVE:
		{
			TextPos pos(cmd->x, cmd->y);
			if ( cursorpos.x != pos.x || cursorpos.y != pos.y )
			{
				if ( IsTextPosBeforeTextPos(cursorpos, render_from) )
					render_from = cursorpos;
				if ( IsTextPosAfterTextPos(cursorpos, render_to) )
					render_to = cursorpos;
				cursorpos = pos;
				if ( IsTextPosBeforeTextPos(cursorpos, render_from) )
					render_from = cursorpos;
				if ( IsTextPosAfterTextPos(cursorpos, render_to) )
					render_to = cursorpos;
			}
		} break;
		case TEXTBUFCMD_MOVE:
		{
			TextPos to(cmd->to_x, cmd->to_y);
			TextPos from(cmd->from_x, cmd->from_y);
			size_t numchars = cmd->val;
			DoMove(to, from, numchars);
			TextPos toend = AddToPosition(to, numchars);
			if ( IsTextPosBeforeTextPos(to, render_from) )
				render_from = to;
			if ( IsTextPosAfterTextPos(toend, render_to) )
				render_to = toend;
		} break;
		case TEXTBUFCMD_FILL:
		{
			TextPos from(cmd->from_x, cmd->from_y);
			TextPos to(cmd->to_x, cmd->to_y);
			DoFill(from, to, cmd->c, cmd->attr);
			if ( IsTextPosBeforeTextPos(from, render_from) )
				render_from = from;
			if ( IsTextPosAfterTextPos(to, render_to) )
				render_to = to;
		} break;
		case TEXTBUFCMD_SCROLL:
		{
			ssize_t off = cmd->scroll_offset;
			DoScroll(off, cmd->c);
			render_from = {0, 0};
			render_to = {columns-1, rows-1};
		} break;
	}
}

void LFBTextBuffer::RenderThread()
{
	queue_is_paused = false;
	size_t offset = 0;
	size_t amount = 0;
	bool exit_requested = false;
	bool sync_requested = false;
	bool pause_requested = false;

	while ( true )
	{
		kthread_mutex_lock(&queue_lock);

		if ( queue_used == queue_length && amount )
			kthread_cond_signal(&queue_not_full);

		queue_used -= amount;
		queue_offset = (queue_offset + amount) % queue_length;

		if ( !queue_used )
		{
			if ( exit_requested )
			{
				queue_thread = false;
				kthread_cond_signal(&queue_exit);
				kthread_mutex_unlock(&queue_lock);
				return;
			}

			if ( sync_requested )
			{
				kthread_cond_signal(&queue_sync);
				sync_requested = false;
			}

			if ( pause_requested )
			{
				queue_is_paused = true;
				kthread_cond_signal(&queue_paused);
				while ( queue_is_paused )
					kthread_cond_wait(&queue_resume, &queue_lock);
				pause_requested = false;
			}
		}

		while ( !queue_used )
			kthread_cond_wait(&queue_not_empty, &queue_lock);

		amount = queue_used;
		offset = queue_offset;

		kthread_mutex_unlock(&queue_lock);

		execute_amount = amount;

		kthread_mutex_lock(&execute_lock);

		TextPos render_from(columns - 1, rows - 1);
		TextPos render_to(0, 0);

		for ( size_t i = 0; i < amount; i++ )
		{
			TextBufferCmd* cmd = &queue[(offset + i) % queue_length];
			ExecuteCommand(cmd, exit_requested, sync_requested, pause_requested, render_from, render_to);
		}

		kthread_mutex_unlock(&execute_lock);

		if ( !IsTextPosBeforeTextPos(render_to, render_from) )
			RenderRange(render_from, render_to);
	}
}

bool LFBTextBuffer::EmergencyIsImpaired()
{
	return !emergency_state;
}

bool LFBTextBuffer::EmergencyRecoup()
{
	if ( !emergency_state )
		emergency_state = true;

	if ( !kthread_mutex_trylock(&queue_lock) )
		return false;
	kthread_mutex_unlock(&queue_lock);

	if ( !kthread_mutex_trylock(&execute_lock) )
	{
		for ( size_t i = 0; i < execute_amount; i++ )
		{
			TextBufferCmd* cmd = &queue[(queue_offset + i) % queue_length];
			if ( !IsCommandIdempotent(cmd) )
				return false;
		}
	}
	else
		kthread_mutex_unlock(&execute_lock);

	TextPos render_from(0, 0);
	TextPos render_to(columns - 1, rows - 1);

	for ( size_t i = 0; i < queue_used; i++ )
	{
		bool exit_requested = false;
		bool sync_requested = false;
		bool pause_requested = false;
		TextBufferCmd* cmd = &queue[(queue_offset + i) % queue_length];
		ExecuteCommand(cmd, exit_requested, sync_requested, pause_requested,
		               render_from, render_to);
	}

	queue_used = 0;
	queue_offset = 0;

	RenderRange(render_from, render_to);

	return true;
}

void LFBTextBuffer::EmergencyReset()
{
	// TODO: Reset everything here!

	Fill(TextPos{0, 0}, TextPos{columns-1, rows-1}, TextChar{0, 0}, 0);
	SetCursorPos(TextPos{0, 0});
}

} // namespace Sortix

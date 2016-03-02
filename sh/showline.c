/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * showline.c
 * Display a line on the terminal.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "showline.h"

struct wincurpos predict_cursor(struct cursor_predict* cursor_predict,
                                struct wincurpos wcp,
                                struct winsize ws,
                                wchar_t c)
{
	if ( c == L'\0' )
		return wcp;

	if ( cursor_predict->escaped )
	{
		if ( (L'a' <= c && c <= L'z') || (L'A' <= c && c <= L'Z') )
			cursor_predict->escaped = false;
		return wcp;
	}

	if ( c == L'\e' )
	{
		cursor_predict->escaped = true;
		return wcp;
	}

	if ( c == L'\n' || ws.ws_col <= wcp.wcp_col + 1 )
	{
		wcp.wcp_col = 0;
		if ( wcp.wcp_row + 1 < ws.ws_row )
			wcp.wcp_row++;
	}
	else
	{
		wcp.wcp_col++;
	}

	return wcp;
}

bool predict_will_scroll(struct cursor_predict cursor_predict,
                         struct wincurpos wcp,
                         struct winsize ws,
                         wchar_t c)
{
	if ( c == L'\0' )
		return false;
	if ( cursor_predict.escaped )
		return false;
	return (c == L'\n' || ws.ws_col <= wcp.wcp_col + 1) &&
	       !(wcp.wcp_row + 1 < ws.ws_row);
}

void show_line_begin(struct show_line* show_state, int out_fd)
{
	memset(show_state, 0, sizeof(*show_state));
	show_state->out_fd = out_fd;
	show_state->current_line = NULL;
	show_state->current_cursor = 0;
	tcgetwincurpos(out_fd, &show_state->wcp_start);
	show_state->wcp_current = show_state->wcp_start;
	tcgetwinsize(show_state->out_fd, &show_state->ws);
}

bool show_line_is_weird(const char* line)
{
	for ( size_t i = 0; line[i]; i++ )
	{
		if ( line[i] == '\e' )
		{
			i++;
			if ( line[i] != '[' )
				return true;
			i++;
			while ( ('0' <= line[i] && line[i] <= '9') || line[i] == ';' )
				i++;
			switch ( line[i] )
			{
			case 'm': break;
			default: return true;
			}
			continue;
		}

		switch ( line[i] )
		{
		case '\a': return true;
		case '\b': return true;
		case '\f': return true;
		case '\r': return true;
		case '\t': return true; // TODO: This isn't weird.
		case '\v': return true;
		default: break;
		}
	}

	return false;
}

void show_line_change_cursor(struct show_line* show_state, struct wincurpos wcp)
{
	if ( wcp.wcp_col == show_state->wcp_current.wcp_col &&
	     wcp.wcp_row == show_state->wcp_current.wcp_row )
		return;

	if ( wcp.wcp_col == 0 )
		dprintf(show_state->out_fd, "\e[%zuH", wcp.wcp_row + 1);
	else
		dprintf(show_state->out_fd, "\e[%zu;%zuH", wcp.wcp_row + 1, wcp.wcp_col+ 1);

	show_state->wcp_current = wcp;
}

bool show_line_optimized(struct show_line* show_state, const char* line, size_t cursor)
{
	struct winsize ws = show_state->ws;

	mbstate_t old_ps;
	mbstate_t new_ps;
	memset(&old_ps, 0, sizeof(old_ps));
	memset(&new_ps, 0, sizeof(new_ps));
	struct wincurpos old_wcp = show_state->wcp_start;
	struct wincurpos new_wcp = show_state->wcp_start;
	struct cursor_predict old_cursor_predict;
	struct cursor_predict new_cursor_predict;
	memset(&old_cursor_predict, 0, sizeof(old_cursor_predict));
	memset(&new_cursor_predict, 0, sizeof(new_cursor_predict));
	size_t old_line_offset = 0;
	size_t new_line_offset = 0;
	const char* old_line = show_state->current_line;
	const char* new_line = line;

	struct wincurpos cursor_wcp = show_state->wcp_start;

	while ( true )
	{
		if ( cursor == new_line_offset )
			cursor_wcp = new_wcp;

		wchar_t old_wc;
		wchar_t new_wc;

		size_t old_num_bytes = mbrtowc(&old_wc, old_line + old_line_offset, SIZE_MAX, &old_ps);
		size_t new_num_bytes = mbrtowc(&new_wc, new_line + new_line_offset, SIZE_MAX, &new_ps);
		assert(old_num_bytes != (size_t) -2);
		assert(new_num_bytes != (size_t) -2);
		assert(old_num_bytes != (size_t) -1);
		assert(new_num_bytes != (size_t) -1);
		if ( old_num_bytes == 0 && new_num_bytes == 0 )
			break;

		bool will_scroll = predict_will_scroll(new_cursor_predict, new_wcp, ws, new_wc);
		bool can_scroll = show_state->wcp_start.wcp_row != 0;

		if ( will_scroll && !can_scroll )
		{
			if ( new_line_offset < cursor )
				cursor_wcp = new_wcp;
			break;
		}

		if ( predict_will_scroll(old_cursor_predict, old_wcp, ws, old_wc) )
			break;

		struct wincurpos next_old_wcp = predict_cursor(&old_cursor_predict, old_wcp, ws, old_wc);
		struct wincurpos next_new_wcp = predict_cursor(&new_cursor_predict, new_wcp, ws, new_wc);

		if ( old_wc != new_wc ||
		     old_wcp.wcp_row != new_wcp.wcp_row ||
		     old_wcp.wcp_col != new_wcp.wcp_col )
		{
			// TODO: Use a reliable write instead!

			if ( old_wc == L'\n' && new_wc == L'\n' )
			{
				// Good enough as newlines are invisible.
			}
			else if ( old_wc == L'\n' && new_wc != L'\0' )
			{
				show_line_change_cursor(show_state, new_wcp);
				write(show_state->out_fd, new_line + new_line_offset, new_num_bytes);
				show_state->wcp_current = next_new_wcp;
				old_num_bytes = 0;
			}
			else if ( old_wc != L'\0' && new_wc == '\n' )
			{
				show_line_change_cursor(show_state, old_wcp);
				write(show_state->out_fd, " ", 1);
				show_state->wcp_current = next_old_wcp;
				new_num_bytes = 0;
			}
			else if ( old_wc == L'\n' && new_wc == L'\0' )
			{
				// No need to do anything here as newlines are visible.
			}
			else if ( old_wc == L'\0' && new_wc == L'\n' )
			{
				show_line_change_cursor(show_state, new_wcp);
				write(show_state->out_fd, new_line + new_line_offset, new_num_bytes);
				show_state->wcp_current = next_new_wcp;
			}
			else if ( old_wcp.wcp_row != new_wcp.wcp_row ||
			          old_wcp.wcp_col != new_wcp.wcp_col )
					return false;
			else if ( new_wc == L'\0' && old_wc != L'\0' )
			{
				show_line_change_cursor(show_state, old_wcp);
				write(show_state->out_fd, " ", 1);
				show_state->wcp_current = next_old_wcp;
			}
			else if ( new_wc != L'\0' )
			{
				show_line_change_cursor(show_state, new_wcp);
				write(show_state->out_fd, new_line + new_line_offset, new_num_bytes);
				show_state->wcp_current = next_new_wcp;
			}
		}

		if ( will_scroll && can_scroll )
		{
			cursor_wcp.wcp_row--;
			next_old_wcp.wcp_row--;
			show_state->wcp_start.wcp_row--;
		}

		old_wcp = next_old_wcp;
		new_wcp = next_new_wcp;

		old_line_offset += old_num_bytes;
		new_line_offset += new_num_bytes;
	}

	show_line_change_cursor(show_state, cursor_wcp);

	if ( show_state->current_line != line )
	{
		free(show_state->current_line);
		show_state->current_line = strdup(line);
		assert(show_state->current_line);
	}
	show_state->current_cursor = cursor;

	return true;
}

void show_line(struct show_line* show_state, const char* line, size_t cursor)
{
	// TODO: We don't currently invalidate on SIGWINCH.
	struct winsize ws;
	tcgetwinsize(show_state->out_fd, &ws);
	if ( ws.ws_col != show_state->ws.ws_col ||
	     ws.ws_row != show_state->ws.ws_row )
	{
		// TODO: What if wcp_start isn't inside the window any longer?
		show_state->invalidated = true;
		show_state->ws = ws;
	}

	// Attempt to do an optimized line re-rendering reusing the characters
	// already present on the console. Bail out if this turns out to be harder
	// than expected and re-render everything from scratch instead.
	if ( !show_state->invalidated &&
	     show_state->current_line &&
	     !show_line_is_weird(show_state->current_line) &&
	     !show_line_is_weird(line) )
	{
		if ( show_line_optimized(show_state, line, cursor) )
			return;
		show_state->invalidated = true;
	}

	show_line_change_cursor(show_state, show_state->wcp_start);

	dprintf(show_state->out_fd, "\e[m");

	if ( show_state->invalidated || show_state->current_line )
		dprintf(show_state->out_fd, "\e[0J");

	struct cursor_predict cursor_predict;
	memset(&cursor_predict, 0, sizeof(cursor_predict));
	struct wincurpos wcp = show_state->wcp_start;
	struct wincurpos cursor_wcp = wcp;

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	for ( size_t i = 0; true; )
	{
		if ( cursor == i )
			cursor_wcp = wcp;
		wchar_t wc;
		size_t num_bytes = mbrtowc(&wc, line + i, SIZE_MAX, &ps);
		assert(num_bytes != (size_t) -2);
		assert(num_bytes != (size_t) -1);
		if ( num_bytes == 0 )
			break;
		bool will_scroll = predict_will_scroll(cursor_predict, wcp, ws, wc);
		bool can_scroll = show_state->wcp_start.wcp_row != 0;
		if ( will_scroll && !can_scroll )
		{
			if ( i < cursor )
				cursor_wcp = wcp;
			break;
		}
		// TODO: Use a reliable write.
		write(show_state->out_fd, line + i, num_bytes);
		if ( will_scroll && can_scroll )
		{
			cursor_wcp.wcp_row--;
			show_state->wcp_start.wcp_row--;
		}
		wcp = predict_cursor(&cursor_predict, wcp, ws, wc);
		i += num_bytes;
	}

	dprintf(show_state->out_fd, "\e[%zu;%zuH",
		cursor_wcp.wcp_row + 1,
		cursor_wcp.wcp_col + 1);

	show_state->wcp_current = wcp;

	if ( show_state->current_line != line )
	{
		free(show_state->current_line);
		show_state->current_line = strdup(line);
		assert(show_state->current_line);
	}
	show_state->current_cursor = cursor;

	show_state->invalidated = false;
}

void show_line_clear(struct show_line* show_state)
{
	dprintf(show_state->out_fd, "\e[H\e[2J");

	show_state->wcp_start.wcp_row = 0;
	show_state->wcp_start.wcp_col = 0;
	show_state->invalidated = true;

	show_line(show_state, show_state->current_line, strlen(show_state->current_line));
}

void show_line_abort(struct show_line* show_state)
{
	free(show_state->current_line);
	show_state->current_line = NULL;
	show_state->current_cursor = 0;
}

void show_line_finish(struct show_line* show_state)
{
	show_line(show_state, show_state->current_line, strlen(show_state->current_line));
	dprintf(show_state->out_fd, "\n");

	show_line_abort(show_state);
}

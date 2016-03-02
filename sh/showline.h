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
 * showline.h
 * Display a line on the terminal.
 */

#ifndef SHOWLINE_H
#define SHOWLINE_H

#include <stddef.h>
#include <termios.h>

// TODO: Predict the terminal colors as well!
struct cursor_predict
{
	bool escaped;
};

struct show_line
{
	struct wincurpos wcp_start;
	struct wincurpos wcp_current;
	struct winsize ws;
	int out_fd;
	char* current_line;
	size_t current_cursor;
	bool invalidated;
};

struct wincurpos predict_cursor(struct cursor_predict* cursor_predict,
                                struct wincurpos wcp,
                                struct winsize ws,
                                wchar_t c);
bool predict_will_scroll(struct cursor_predict cursor_predict,
                         struct wincurpos wcp,
                         struct winsize ws,
                         wchar_t c);
void show_line_begin(struct show_line* show_state, int out_fd);
bool show_line_is_weird(const char* line);
void show_line_change_cursor(struct show_line* show_state, struct wincurpos wcp);
bool show_line_optimized(struct show_line* show_state, const char* line, size_t cursor);
void show_line(struct show_line* show_state, const char* line, size_t cursor);
void show_line_clear(struct show_line* show_state);
void show_line_abort(struct show_line* show_state);
void show_line_finish(struct show_line* show_state);

#endif

/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    showline.h
    Display a line on the terminal.

*******************************************************************************/

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

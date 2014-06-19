/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    sh.cpp
    A hacky Sortix shell.

*******************************************************************************/

#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <ioleast.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

static const unsigned int NORMAL_TERMMODE =
	TERMMODE_UNICODE |
	TERMMODE_SIGNAL |
	TERMMODE_UTF8 |
	TERMMODE_LINEBUFFER |
	TERMMODE_ECHO;

const char* builtin_commands[] =
{
	"cd",
	"exit",
	"unset",
	"clearenv",
	(const char*) NULL,
};

// TODO: Predict the terminal colors as well!
struct cursor_predict
{
	bool escaped;
};

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

	free(show_state->current_line);
	show_state->current_line = strdup(line);
	assert(show_state->current_line);
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

	free(show_state->current_line);
	show_state->current_line = strdup(line);
	assert(show_state->current_line);
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

struct edit_line
{
	const char* ps1;
	const char* ps2;
	struct show_line show_state;
	wchar_t* line;
	size_t line_offset;
	size_t line_used;
	size_t line_length;
	char** history;
	size_t history_offset;
	size_t history_used;
	size_t history_length;
	size_t history_target;
	void* check_input_incomplete_context;
	bool (*check_input_incomplete)(void*, const char*);
	void* trap_eof_opportunity_context;
	void (*trap_eof_opportunity)(void*);
	void* complete_context;
	size_t (*complete)(char***, size_t*, size_t*, void*, const char*, size_t);
	int in_fd;
	int out_fd;
	bool editing;
	bool abort_editing;
	bool eof_condition;
	bool double_tab;
	// TODO: Should these be stored here, or outside the line editing context?
	bool left_control;
	bool right_control;
};

void edit_line_show(struct edit_line* edit_state)
{
	size_t line_length = 0;

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));

	line_length += strlen(edit_state->ps1);

	for ( size_t i = 0; i < edit_state->line_used; i++ )
	{
		char mb[MB_CUR_MAX];
		line_length += wcrtomb(mb, edit_state->line[i], &ps);
		if ( edit_state->line[i] == L'\n' )
			line_length += strlen(edit_state->ps2);
	}

	char* line = (char*) malloc(line_length + 1);
	assert(line);

	size_t cursor = 0;
	size_t line_offset = 0;
	memset(&ps, 0, sizeof(ps));

	strcpy(line + line_offset, edit_state->ps1);
	line_offset += strlen(edit_state->ps1);

	for ( size_t i = 0; i < edit_state->line_used; i++ )
	{
		if ( edit_state->line_offset == i )
			cursor = line_offset;
		line_offset += wcrtomb(line + line_offset, edit_state->line[i], &ps);
		if ( edit_state->line[i] == L'\n' )
		{
			strcpy(line + line_offset, edit_state->ps2);
			line_offset += strlen(edit_state->ps2);
		}
	}

	if ( edit_state->line_offset == edit_state->line_used )
		cursor = line_offset;

	line[line_offset] = '\0';

	show_line(&edit_state->show_state, line, cursor);

	free(line);
}

char* edit_line_result(struct edit_line* edit_state)
{
	size_t result_length = 0;

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));

	for ( size_t i = 0; i < edit_state->line_used; i++ )
	{
		char mb[MB_CUR_MAX];
		result_length += wcrtomb(mb, edit_state->line[i], &ps);
	}

	char* result = (char*) malloc(result_length + 1);
	if ( !result )
		return NULL;
	size_t result_offset = 0;

	memset(&ps, 0, sizeof(ps));

	for ( size_t i = 0; i < edit_state->line_used; i++ )
		result_offset += wcrtomb(result + result_offset, edit_state->line[i], &ps);

	result[result_offset] = '\0';

	return result;
}

bool edit_line_can_finish(struct edit_line* edit_state)
{
	if ( !edit_state->check_input_incomplete )
		return true;
	char* line = edit_line_result(edit_state);
	assert(line);
	bool result = !edit_state->check_input_incomplete(
		edit_state->check_input_incomplete_context, line);
	free(line);
	return result;
}

void edit_line_append_history(struct edit_line* edit_state, const char* line)
{
	if ( edit_state->history_used == edit_state->history_length )
	{
		size_t new_length = 2 * edit_state->history_length;
		if ( new_length == 0 )
			new_length = 16;
		// TODO: Use reallocarray instead of realloc.
		size_t new_size = sizeof(char*) * new_length;
		char** new_history = (char**) realloc(edit_state->history, new_size);
		assert(new_history);
		edit_state->history = new_history;
		edit_state->history_length = new_length;
	}

	size_t history_index = edit_state->history_used++;
	edit_state->history[history_index] = strdup(line);
	assert(edit_state->history[history_index]);
}

void edit_line_type_use_record(struct edit_line* edit_state, const char* record)
{
	free(edit_state->line);
	edit_state->line_offset = 0;
	edit_state->line_used = 0;
	edit_state->line_length = 0;

	size_t line_length;

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));

	size_t record_offset = 0;
	for ( line_length = 0; true; line_length++ )
	{
		size_t num_bytes = mbrtowc(NULL, record + record_offset, SIZE_MAX, &ps);
		assert(num_bytes != (size_t) -2);
		assert(num_bytes != (size_t) -1);
		if ( num_bytes == 0 )
			break;
		record_offset += num_bytes;
	}

	// TODO: Avoid multiplication overflow.
	wchar_t* line = (wchar_t*) malloc(sizeof(wchar_t) * line_length);
	assert(line);
	size_t line_used;

	memset(&ps, 0, sizeof(ps));

	record_offset = 0;
	for ( line_used = 0; line_used < line_length; line_used++ )
	{
		size_t num_bytes = mbrtowc(&line[line_used], record + record_offset, SIZE_MAX, &ps);
		assert(num_bytes != (size_t) -2);
		assert(num_bytes != (size_t) -1);
		assert(num_bytes != (size_t) 0);
		record_offset += num_bytes;
	}

	edit_state->line = line;
	edit_state->line_offset = line_used;
	edit_state->line_used = line_used;
	edit_state->line_length = line_length;
}

void edit_line_type_history_save_at(struct edit_line* edit_state, size_t index)
{
	assert(index <= edit_state->history_used);

	char* saved_line = edit_line_result(edit_state);
	assert(saved_line);
	if ( index == edit_state->history_used )
	{
		edit_line_append_history(edit_state, saved_line);
		free(saved_line);
	}
	else
	{
		free(edit_state->history[index]);
		edit_state->history[index] = saved_line;
	}
}

void edit_line_type_history_save_current(struct edit_line* edit_state)
{
	edit_line_type_history_save_at(edit_state, edit_state->history_offset);
}

void edit_line_type_history_prev(struct edit_line* edit_state)
{
	if ( edit_state->history_offset == 0 )
		return;

	edit_line_type_history_save_current(edit_state);

	const char* record = edit_state->history[--edit_state->history_offset];
	assert(record);
	edit_line_type_use_record(edit_state, record);
}

void edit_line_type_history_next(struct edit_line* edit_state)
{
	if ( edit_state->history_used - edit_state->history_offset <= 1 )
		return;

	edit_line_type_history_save_current(edit_state);

	const char* record = edit_state->history[++edit_state->history_offset];
	assert(record);
	edit_line_type_use_record(edit_state, record);
}

void edit_line_type_codepoint(struct edit_line* edit_state, wchar_t wc)
{
	if ( wc == L'\n' && edit_line_can_finish(edit_state))
	{
		if ( edit_state->line_used )
			edit_line_type_history_save_at(edit_state, edit_state->history_target);
		edit_state->editing = false;
		return;
	}

	if ( edit_state->line_used == edit_state->line_length )
	{
		size_t new_length = 2 * edit_state->line_length;
		if ( !new_length )
			new_length = 16;
		// TODO: Use reallocarray instead of realloc.
		size_t new_size = sizeof(wchar_t) * new_length;
		wchar_t* new_line = (wchar_t*) realloc(edit_state->line, new_size);
		assert(new_line);
		edit_state->line = new_line;
		edit_state->line_length = new_length;
	}

	assert(edit_state->line_offset <= edit_state->line_used);
	assert(edit_state->line_used <= edit_state->line_length);

	for ( size_t i = edit_state->line_used; i != edit_state->line_offset; i-- )
		edit_state->line[i] = edit_state->line[i-1];

	edit_state->line[edit_state->line_used++, edit_state->line_offset++] = wc;

	assert(edit_state->line_offset <= edit_state->line_used);
	assert(edit_state->line_used <= edit_state->line_length);
}

void line_edit_type_home(struct edit_line* edit_state)
{
	edit_state->line_offset = 0;
}

void line_edit_type_left(struct edit_line* edit_state)
{
	if ( edit_state->line_offset == 0 )
		return;
	edit_state->line_offset--;
}

void line_edit_type_right(struct edit_line* edit_state)
{
	if ( edit_state->line_offset == edit_state->line_used )
		return;
	edit_state->line_offset++;
}

void line_edit_type_end(struct edit_line* edit_state)
{
	edit_state->line_offset = edit_state->line_used;
}

void line_edit_type_backspace(struct edit_line* edit_state)
{
	if ( edit_state->line_offset == 0 )
		return;
	edit_state->line_used--;
	edit_state->line_offset--;
	for ( size_t i = edit_state->line_offset; i < edit_state->line_used; i++ )
		edit_state->line[i] = edit_state->line[i+1];
}

void line_edit_type_previous_word(struct edit_line* edit_state)
{
	while ( edit_state->line_offset &&
	        iswspace(edit_state->line[edit_state->line_offset-1]) )
		edit_state->line_offset--;
	while ( edit_state->line_offset &&
	        !iswspace(edit_state->line[edit_state->line_offset-1]) )
		edit_state->line_offset--;
}

void line_edit_type_next_word(struct edit_line* edit_state)
{
	while ( edit_state->line_offset != edit_state->line_used &&
	        iswspace(edit_state->line[edit_state->line_offset]) )
		edit_state->line_offset++;
	while ( edit_state->line_offset != edit_state->line_used &&
	        !iswspace(edit_state->line[edit_state->line_offset]) )
		edit_state->line_offset++;
}

void line_edit_type_delete(struct edit_line* edit_state)
{
	if ( edit_state->line_offset == edit_state->line_used )
		return;
	edit_state->line_used--;
	for ( size_t i = edit_state->line_offset; i < edit_state->line_used; i++ )
		edit_state->line[i] = edit_state->line[i+1];
}

void line_edit_type_eof_or_delete(struct edit_line* edit_state)
{
	if ( edit_state->line_used )
		return line_edit_type_delete(edit_state);
	edit_state->editing = false;
	edit_state->eof_condition = true;
	if ( edit_state->trap_eof_opportunity )
		edit_state->trap_eof_opportunity(edit_state->trap_eof_opportunity_context);
}

void edit_line_type_interrupt(struct edit_line* edit_state)
{
	dprintf(edit_state->out_fd, "^C\n");
	edit_state->editing = false;
	edit_state->abort_editing = true;
}

void edit_line_type_kill_after(struct edit_line* edit_state)
{
	while ( edit_state->line_offset < edit_state->line_used )
		line_edit_type_delete(edit_state);
}

void edit_line_type_kill_before(struct edit_line* edit_state)
{
	while ( edit_state->line_offset )
		line_edit_type_backspace(edit_state);
}

void edit_line_type_clear(struct edit_line* edit_state)
{
	show_line_clear(&edit_state->show_state);
}

void edit_line_type_delete_word_before(struct edit_line* edit_state)
{
	while ( edit_state->line_offset &&
	        iswspace(edit_state->line[edit_state->line_offset-1]) )
		line_edit_type_backspace(edit_state);
	while ( edit_state->line_offset &&
	        !iswspace(edit_state->line[edit_state->line_offset-1]) )
		line_edit_type_backspace(edit_state);
}

int edit_line_completion_sort(const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char**) a_ptr;
	const char* b = *(const char**) b_ptr;
	return strcmp(a, b);
}

void edit_line_type_complete(struct edit_line* edit_state)
{
	if ( !edit_state->complete )
		return;

	char* partial = edit_line_result(edit_state);
	if ( !partial )
		return;

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));

	size_t complete_at = 0;
	for ( size_t i = 0; i < edit_state->line_offset; i++ )
	{
		char mb[MB_CUR_MAX];
		size_t num_bytes = wcrtomb(mb, edit_state->line[i], &ps);
		assert(num_bytes != (size_t) -1);
		assert(num_bytes != (size_t)  0);
		complete_at += num_bytes;
	}

	char** completions;
	size_t used_before;
	size_t used_after;
	size_t num_completions = edit_state->complete(
		&completions,
		&used_before,
		&used_after,
		edit_state->complete_context,
		partial,
		complete_at);

	qsort(completions, num_completions, sizeof(char*), edit_line_completion_sort);

	size_t lcp = 0;
	bool similar = true;
	while ( num_completions && similar )
	{
		char c = completions[0][lcp];
		if ( c == '\0' )
			break;
		for ( size_t i = 1; similar && i < num_completions; i++ )
		{
			if ( completions[i][lcp] != c )
				similar = false;
		}
		if ( similar )
			lcp++;
	}

	bool prefix_ends_with_slash = false;
	memset(&ps, 0, sizeof(ps));
	for ( size_t i = 0; i < lcp; )
	{
		const char* completion = completions[0];
		wchar_t wc;
		size_t num_bytes = mbrtowc(&wc, completion + i, lcp - i, &ps);
		if ( num_bytes == (size_t) -2 )
			break;
		assert(num_bytes != (size_t) -1);
		assert(num_bytes != (size_t)  0);
		edit_line_type_codepoint(edit_state, wc);
		prefix_ends_with_slash = wc == L'/';
		i += num_bytes;
	}

	if ( num_completions == 1 && !prefix_ends_with_slash )
	{
		edit_line_type_codepoint(edit_state, ' ');
	}

	if ( 2 <= num_completions && lcp == 0 && edit_state->double_tab )
	{
		bool first = true;
		for ( size_t i = 0; i < num_completions; i++ )
		{
			const char* completion = completions[i];
			size_t length = used_before + strlen(completion) + used_after;
			if ( !length )
				continue;
			if ( first )
				show_line_finish(&edit_state->show_state);
			// TODO: Use a reliable write.
			if ( !first )
				write(edit_state->out_fd, " ", 1);
			write(edit_state->out_fd, partial + complete_at - used_before, used_before);
			write(edit_state->out_fd, completion, strlen(completion));
			write(edit_state->out_fd, partial + complete_at, used_after);
			first = false;
		}
		if ( !first)
		{
			write(edit_state->out_fd, "\n", 1);
			show_line_begin(&edit_state->show_state, edit_state->out_fd);
			edit_line_show(edit_state);
		}
	}

	edit_state->double_tab = true;

	(void) used_before;
	(void) used_after;

	for ( size_t i = 0; i < num_completions; i++ )
		free(completions[i]);
	free(completions);

	free(partial);
}

void edit_line_kbkey(struct edit_line* edit_state, int kbkey)
{
	if ( kbkey != KBKEY_TAB && kbkey != -KBKEY_TAB )
		edit_state->double_tab = false;

	if ( edit_state->left_control || edit_state->right_control )
	{
		switch ( kbkey )
		{
		case KBKEY_LEFT: line_edit_type_previous_word(edit_state); return;
		case KBKEY_RIGHT: line_edit_type_next_word(edit_state); return;
		};
	}

	switch ( kbkey )
	{
	case KBKEY_HOME: line_edit_type_home(edit_state); return;
	case KBKEY_LEFT: line_edit_type_left(edit_state); return;
	case KBKEY_RIGHT: line_edit_type_right(edit_state); return;
	case KBKEY_UP: edit_line_type_history_prev(edit_state); return;
	case KBKEY_DOWN: edit_line_type_history_next(edit_state); return;
	case KBKEY_END: line_edit_type_end(edit_state); return;
	case KBKEY_BKSPC: line_edit_type_backspace(edit_state); return;
	case KBKEY_DELETE: line_edit_type_delete(edit_state); return;
	case KBKEY_TAB: edit_line_type_complete(edit_state); return;
	case -KBKEY_LCTRL: edit_state->left_control = false; return;
	case +KBKEY_LCTRL: edit_state->left_control = true; return;
	case -KBKEY_RCTRL: edit_state->right_control = false; return;
	case +KBKEY_RCTRL: edit_state->right_control = true; return;
	};
}

void edit_line_codepoint(struct edit_line* edit_state, wchar_t wc)
{
	if ( (edit_state->left_control || edit_state->right_control) &&
	     ((L'a' <= wc && wc <= L'z') || (L'A' <= wc && wc <= L'Z')) )
	{
		if ( wc == L'a' || wc == L'A' )
			line_edit_type_home(edit_state);
		if ( wc == L'b' || wc == L'B' )
			line_edit_type_left(edit_state);
		if ( wc == L'c' || wc == L'C' )
			edit_line_type_interrupt(edit_state);
		if ( wc == L'd' || wc == L'D' )
			line_edit_type_eof_or_delete(edit_state);
		if ( wc == L'e' || wc == L'E' )
			line_edit_type_end(edit_state);
		if ( wc == L'f' || wc == L'F' )
			line_edit_type_right(edit_state);
		if ( wc == L'k' || wc == L'K' )
			edit_line_type_kill_after(edit_state);
		if ( wc == L'l' || wc == L'L' )
			show_line_clear(&edit_state->show_state);
		if ( wc == L'u' || wc == L'U' )
			edit_line_type_kill_before(edit_state);
		if ( wc == L'w' || wc == L'W' )
			edit_line_type_delete_word_before(edit_state);
		return;
	}

	if ( wc == L'\b' )
		return;
	if ( wc == L'\t' )
		return;

	edit_line_type_codepoint(edit_state, wc);
}

void edit_line(struct edit_line* edit_state)
{
	edit_state->editing = true;
	edit_state->abort_editing = false;
	edit_state->eof_condition = false;
	edit_state->double_tab = false;

	free(edit_state->line);
	edit_state->line = NULL;
	edit_state->line_offset = 0;
	edit_state->line_used = 0;
	edit_state->line_length = 0;
	edit_state->history_offset = edit_state->history_used;
	edit_state->history_target = edit_state->history_used;

	settermmode(edit_state->in_fd, TERMMODE_KBKEY | TERMMODE_UNICODE);

	show_line_begin(&edit_state->show_state, edit_state->out_fd);

	while ( edit_state->editing )
	{
		edit_line_show(edit_state);

		uint32_t codepoint;
		if ( read(0, &codepoint, sizeof(codepoint)) != sizeof(codepoint) )
		{
			edit_state->eof_condition = true;
			edit_state->abort_editing = true;
			break;
		}

		if ( int kbkey = KBKEY_DECODE(codepoint) )
			edit_line_kbkey(edit_state, kbkey);
		else
			edit_line_codepoint(edit_state, (wchar_t) codepoint);
	}

	if ( edit_state->abort_editing )
		show_line_abort(&edit_state->show_state);
	else
	{
		edit_line_show(edit_state);
		show_line_finish(&edit_state->show_state);
	}

	settermmode(edit_state->in_fd, NORMAL_TERMMODE);
}

int status = 0;

char* strdup_safe(const char* string)
{
	return string ? strdup(string) : NULL;
}

const char* getenv_safe(const char* name, const char* def = "")
{
	const char* ret = getenv(name);
	return ret ? ret : def;
}

void update_pwd()
{
	char* wd = get_current_dir_name();
	setenv("PWD", wd ? wd : "?", 1);
	free(wd);
}

void update_env()
{
	char str[128];
	struct winsize ws;
	if ( tcgetwinsize(0, &ws) == 0 )
	{
		snprintf(str, sizeof(str), "%zu", ws.ws_col);
		setenv("COLUMNS", str, 1);
		snprintf(str, sizeof(str), "%zu", ws.ws_row);
		setenv("LINES", str, 1);
	}
}

bool matches_simple_pattern(const char* string, const char* pattern)
{
	size_t wildcard_index = strcspn(pattern, "*");
	if ( !pattern[wildcard_index] )
		return strcmp(string, pattern) == 0;
	if ( pattern[0] == '*' && string[0] == '.' )
		return false;
	size_t string_length = strlen(string);
	size_t pattern_length = strlen(pattern);
	size_t pattern_last = pattern_length - (wildcard_index + 1);
	return strncmp(string, pattern, wildcard_index) == 0 &&
	       strcmp(string + string_length - pattern_last,
	              pattern + wildcard_index + 1) == 0;
}

int runcommandline(const char** tokens, bool* exitexec, bool interactive)
{
	int result = 127;
	size_t cmdnext = 0;
	size_t cmdstart;
	size_t cmdend;
	bool lastcmd = false;
	int pipein = 0;
	int pipeout = 1;
	int pipeinnext = 0;
	char** argv;
	size_t cmdlen;
	const char* execmode;
	const char* outputfile;
	pid_t childpid;
	pid_t pgid = -1;
	bool internal;
	int internalresult;
	size_t num_tokens = 0;
	while ( tokens[num_tokens] )
		num_tokens++;
readcmd:
	// Collect any pending zombie processes.
	while ( 0 < waitpid(-1, NULL, WNOHANG) );

	cmdstart = cmdnext;
	for ( cmdend = cmdstart; true; cmdend++ )
	{
		const char* token = tokens[cmdend];
		if ( !token ||
		     strcmp(token, ";") == 0 ||
		     strcmp(token, "&") == 0 ||
		     strcmp(token, "|") == 0 ||
		     strcmp(token, ">") == 0 ||
		     strcmp(token, ">>") == 0 ||
		     false )
		{
			break;
		}
	}

	cmdlen = cmdend - cmdstart;
	if ( !cmdlen ) { fprintf(stderr, "expected command\n"); goto out; }
	execmode = tokens[cmdend];
	if ( !execmode ) { lastcmd = true; execmode = ";"; }
	tokens[cmdend] = NULL;

	if ( strcmp(execmode, "|") == 0 )
	{
		int pipes[2];
		if ( pipe(pipes) ) { perror("pipe"); goto out; }
		if ( pipeout != 1 ) { close(pipeout); } pipeout = pipes[1];
		if ( pipeinnext != 0 ) { close(pipeinnext); } pipeinnext = pipes[0];
	}

	outputfile = NULL;
	if ( strcmp(execmode, ">") == 0 || strcmp(execmode, ">>") == 0 )
	{
		outputfile = tokens[cmdend+1];
		if ( !outputfile ) { fprintf(stderr, "expected filename\n"); goto out; }
		const char* nexttok = tokens[cmdend+2];
		if ( nexttok ) { fprintf(stderr, "too many filenames\n"); goto out; }
	}

	for ( size_t i = cmdstart; i < cmdend; i++ )
	{
		const char* pattern = tokens[i];
		size_t wildcard_pos = strcspn(pattern, "*");
		if ( !pattern[wildcard_pos] )
			continue;
		bool found_slash = false;
		size_t last_slash = 0;
		for ( size_t i = 0; i < wildcard_pos; i++ )
			if ( pattern[i] == '/' )
				last_slash = i, found_slash = true;
		size_t match_from = found_slash ? last_slash + 1 : 0;
		DIR* dir;
		size_t pattern_prefix = 0;
		if ( !found_slash )
		{
			if ( !(dir = opendir(".")) )
				continue;
		}
		else
		{
			char* dirpath = strdup(pattern);
			if ( !dirpath )
				continue;
			dirpath[last_slash] = '\0';
			pattern_prefix = last_slash + 1;
			dir = opendir(dirpath);
			free(dirpath);
			if ( !dir )
				continue;
		}
		size_t num_inserted = 0;
		size_t last_inserted_index = i;
		while ( struct dirent* entry = readdir(dir) )
		{
			if ( !matches_simple_pattern(entry->d_name, pattern + match_from) )
				continue;
			// TODO: Memory leak.
			char* name = (char*) malloc(pattern_prefix + strlen(entry->d_name) + 1);
			memcpy(name, pattern, pattern_prefix);
			strcpy(name + pattern_prefix, entry->d_name);
			if ( !name )
				continue;
			if ( num_inserted )
			{
				// TODO: Reckless modification of the tokens array.
				for ( size_t n = num_tokens; n != last_inserted_index; n-- )
					tokens[n+1] = tokens[n];
				num_tokens++;
				cmdend++;
			}
			// TODO: Reckless modification of the tokens array.
			tokens[last_inserted_index = i + num_inserted++] = name;
		}
		closedir(dir);
	}

	cmdnext = cmdend + 1;
	argv = (char**) (tokens + cmdstart);

	update_env();
	char statusstr[32];
	snprintf(statusstr, sizeof(statusstr), "%i", status);
	setenv("?", statusstr, 1);

	for ( char** argp = argv; *argp; argp++ )
	{
		char* arg = *argp;
		if ( arg[0] != '$' )
			continue;
		arg = getenv(arg+1);
		if ( !arg )
			arg = (char*) "";
		*argp = arg;
	}

	internal = false;
	internalresult = 0;
	if ( strcmp(argv[0], "cd") == 0 )
	{
		internal = true;
		const char* newdir = getenv_safe("HOME", "/");
		if ( argv[1] ) { newdir = argv[1]; }
		if ( chdir(newdir) )
		{
			error(0, errno, "cd: %s", newdir);
			internalresult = 1;
		}
		update_pwd();
	}
	if ( strcmp(argv[0], "exit") == 0 )
	{
		int exitcode = argv[1] ? atoi(argv[1]) : 0;
		*exitexec = true;
		return exitcode;
	}
	if ( strcmp(argv[0], "unset") == 0 )
	{
		internal = true;
		unsetenv(argv[1] ? argv[1] : "");
	}
	if ( strcmp(argv[0], "clearenv") == 0 )
	{
		internal = true;
		clearenv();
	}

	childpid = internal ? getpid() : fork();
	if ( childpid < 0 ) { perror("fork"); goto out; }
	if ( childpid )
	{
		if ( !internal )
		{
			if ( pgid == -1 )
				pgid = childpid;
			setpgid(childpid, pgid);
		}

		if ( pipein != 0 ) { close(pipein); pipein = 0; }
		if ( pipeout != 1 ) { close(pipeout); pipeout = 1; }
		if ( pipeinnext != 0 ) { pipein = pipeinnext; pipeinnext = 0; }

		if ( strcmp(execmode, "&") == 0 && !tokens[cmdnext] )
		{
			result = 0; goto out;
		}

		if ( strcmp(execmode, "&") == 0 )
			pgid = -1;

		if ( strcmp(execmode, "&") == 0 || strcmp(execmode, "|") == 0 )
		{
			goto readcmd;
		}

		status = internalresult;
		int exitstatus;
		tcsetpgrp(0, pgid);
		if ( !internal && waitpid(childpid, &exitstatus, 0) < 0 )
		{
			perror("waitpid");
			return 127;
		}
		tcsetpgrp(0, getpgid(0));

		// TODO: HACK: Most signals can't kill processes yet.
		if ( WEXITSTATUS(exitstatus) == 128 + SIGINT )
			printf("^C\n");
		if ( WTERMSIG(status) == SIGKILL )
			printf("Killed\n");

		status = WEXITSTATUS(exitstatus);

		if ( strcmp(execmode, ";") == 0 && tokens[cmdnext] && !lastcmd )
		{
			goto readcmd;
		}

		result = status;
		goto out;
	}

	setpgid(0, pgid != -1 ? pgid : 0);

	if ( pipeinnext != 0 ) { close(pipeinnext); }

	if ( pipein != 0 )
	{
		close(0);
		dup(pipein);
		close(pipein);
	}

	if ( pipeout != 1 )
	{
		close(1);
		dup(pipeout);
		close(pipeout);
	}

	if ( outputfile )
	{
		close(1);
		// TODO: Is this logic right or wrong?
		int flags = O_CREAT | O_WRONLY;
		if ( strcmp(execmode, ">") == 0 )
			flags |= O_TRUNC;
		if ( strcmp(execmode, ">>") == 0 )
			flags |= O_APPEND;
		if ( open(outputfile, flags, 0666) < 0 )
		{
			error(127, errno, "%s", outputfile);
		}
	}

	execvp(argv[0], argv);
	if ( interactive && errno == ENOENT )
	{
		int errno_saved = errno;
		execlp("command-not-found", "command-not-found", argv[0], NULL);
		errno = errno_saved;
	}
	error(127, errno, "%s", argv[0]);
	return 127;

out:
	if ( pipein != 0 ) { close(pipein); }
	if ( pipeout != 1 ) { close(pipeout); }
	if ( pipeinnext != 0 ) { close(pipeout); }
	return result;
}

int run_command(char* command,
                bool interactive,
                bool exit_on_error,
                bool* exitexec)
{
	size_t commandused = strlen(command);

	if ( command[0] == '\0' )
		return status;

	if ( strchr(command, '=') && !strchr(command, ' ') && !strchr(command, '\t') )
	{
		const char* key = command;
		char* equal = strchr(command, '=');
		*equal = '\0';
		const char* value = equal + 1;
		if ( setenv(key, value, 1) < 0 )
			error(1, errno, "setenv");
		return status = 0;
	}

	int argc = 0;
	const size_t ARGV_MAX_LENGTH = 2048;
	const char* argv[ARGV_MAX_LENGTH];
	argv[0] = NULL;

	bool lastwasspace = true;
	bool escaped = false;
	for ( size_t i = 0; i <= commandused; i++ )
	{
		switch ( command[i] )
		{
			case '\\':
				if ( !escaped )
				{
					memmove(command + i, command + i + 1, commandused+1 - (i-1));
					i--;
					commandused--;
					escaped = true;
					break;
				}
			case '\0':
			case ' ':
			case '\t':
			case '\n':
				if ( !command[i] || !escaped )
				{
					command[i] = 0;
					lastwasspace = true;
					break;
				}
			default:
				escaped = false;
				if ( lastwasspace )
				{
					if ( argc == ARGV_MAX_LENGTH  )
					{
						fprintf(stderr, "argv max length of %zu entries hit!\n",
						        ARGV_MAX_LENGTH);
						abort();
					}
					argv[argc++] = command + i;
				}
				lastwasspace = false;
		}
	}

	if ( !argv[0] )
		return status;

	argv[argc] = NULL;
	status = runcommandline(argv, exitexec, interactive);
	if ( status && exit_on_error )
		*exitexec = true;
	return status;
}

bool is_shell_input_ready(const char* input)
{
	bool commented = false;
	bool escaped = false;
	for ( size_t i = 0; input[i]; i++ )
	{
		if ( !commented && !escaped && input[i] == '\\' )
			escaped = true;
		else if ( !commented && !escaped && input[i] == '#' )
			commented = true;
		else if ( !commented )
			escaped = false;
	}
	return !escaped;
}

bool does_line_editing_need_another_line(void*, const char* line)
{
	return !is_shell_input_ready(line);
}

bool is_outermost_shell()
{
	const char* shlvl_str = getenv("SHLVL");
	if ( !shlvl_str )
		return true;
	return atol(shlvl_str) <= 1;
}

void on_trap_eof(void* edit_state_ptr)
{
	if ( is_outermost_shell() )
		return;
	struct edit_line* edit_state = (struct edit_line*) edit_state_ptr;
	edit_line_type_codepoint(edit_state, L'e');
	edit_line_type_codepoint(edit_state, L'x');
	edit_line_type_codepoint(edit_state, L'i');
	edit_line_type_codepoint(edit_state, L't');
}

bool is_usual_char_for_completion(char c)
{
	return !isspace(c) &&
	       c != ';' && c != '&' && c != '|' &&
	       c != '<' && c != '>' && c != '#' && c != '$';
}

size_t do_complete(char*** completions_ptr,
                   size_t* used_before_ptr,
                   size_t* used_after_ptr,
                   void*,
                   const char* partial,
                   size_t complete_at)
{
	size_t used_before = 0;
	size_t used_after = 0;

	while ( complete_at - used_before &&
	        is_usual_char_for_completion(partial[complete_at - (used_before+1)]) )
		used_before++;

#if 0
	while ( partial[complete_at + used_after] &&
	        is_usual_char_for_completion(partial[complete_at + used_after]) )
		used_after++;
#endif

	enum complete_type
	{
		COMPLETE_TYPE_FILE,
		COMPLETE_TYPE_EXECUTABLE,
		COMPLETE_TYPE_DIRECTORY,
		COMPLETE_TYPE_PROGRAM,
		COMPLETE_TYPE_VARIABLE,
	};

	enum complete_type complete_type = COMPLETE_TYPE_FILE;

	if ( complete_at - used_before && partial[complete_at - used_before-1] == '$' )
	{
		complete_type = COMPLETE_TYPE_VARIABLE;
		used_before++;
	}
	else
	{
		size_t type_offset = complete_at - used_before;
		while ( type_offset && isspace(partial[type_offset-1]) )
			type_offset--;

		if ( 2 <= type_offset &&
		     strncmp(partial + type_offset - 2, "cd", 2) == 0 &&
		     (type_offset == 2 || !is_usual_char_for_completion(partial[type_offset-2-1])) )
			complete_type = COMPLETE_TYPE_DIRECTORY;
		else if ( !type_offset ||
			      partial[type_offset-1] == ';' ||
			      partial[type_offset-1] == '&' ||
			      partial[type_offset-1] == '|' )
		{
			if ( memchr(partial + complete_at - used_before, '/', used_before) )
				complete_type = COMPLETE_TYPE_EXECUTABLE;
			else
				complete_type = COMPLETE_TYPE_PROGRAM;
		}
	}

	// TODO: Use reallocarray.
	char** completions = (char**) malloc(sizeof(char**) * 1024 /* TODO: HARD-CODED! */);
	size_t num_completions = 0;

	if ( complete_type == COMPLETE_TYPE_PROGRAM ) do
	{
		for ( size_t i = 0; builtin_commands[i]; i++ )
		{
			const char* builtin = builtin_commands[i];
			if ( strncmp(builtin, partial + complete_at - used_before, used_before) != 0 )
				continue;
			// TODO: Add allocation check!
			completions[num_completions++] = strdup(builtin + used_before);
		}
		char* path = strdup_safe(getenv("PATH"));
		if ( !path )
		{
			complete_type = COMPLETE_TYPE_FILE;
			break;
		}
		char* path_input = path;
		char* saved_ptr;
		char* component;
		while ( (component = strtok_r(path_input, " ", &saved_ptr)) )
		{
			if ( DIR* dir = opendir(component) )
			{
				while ( struct dirent* entry = readdir(dir) )
				{
					if ( strncmp(entry->d_name, partial + complete_at - used_before, used_before) != 0 )
						continue;
					if ( used_before == 0 &&  entry->d_name[0] == '.' )
						continue;
					// TODO: Add allocation check!
					completions[num_completions++] = strdup(entry->d_name + used_before);
				}
				closedir(dir);
			}
			path_input = NULL;
		}
		free(path);
	} while ( false );

	if ( complete_type == COMPLETE_TYPE_FILE ||
	     complete_type == COMPLETE_TYPE_EXECUTABLE ||
	     complete_type == COMPLETE_TYPE_DIRECTORY ) do
	{
		const char* pattern = partial + complete_at - used_before;
		size_t pattern_length = used_before;

		char* dirpath_alloc = NULL;
		const char* dirpath;
		if ( !memchr(pattern, '/', pattern_length) )
			dirpath = ".";
		else if ( pattern_length && pattern[pattern_length-1] == '/' )
		{
			dirpath_alloc = strndup(pattern, pattern_length);
			if ( !dirpath_alloc )
				break;
			dirpath = dirpath_alloc;
			pattern += pattern_length;
			pattern_length = 0;
		}
		else
		{
			dirpath_alloc = strndup(pattern, pattern_length);
			if ( !dirpath_alloc )
				break;
			dirpath = dirname(dirpath_alloc);
			const char* last_slash = (const char*) memrchr(pattern, '/', pattern_length);
			size_t last_slash_offset = (uintptr_t) last_slash - (uintptr_t) pattern;
			pattern += last_slash_offset + 1;
			pattern_length -= last_slash_offset + 1;
		}
		used_before = pattern_length;
		DIR* dir = opendir(dirpath);
		if ( !dir )
		{
			free(dirpath_alloc);
			break;
		}
		while ( struct dirent* entry = readdir(dir) )
		{
			if ( strncmp(entry->d_name, pattern, pattern_length) != 0 )
				continue;
			if ( pattern_length == 0 &&  entry->d_name[0] == '.' )
				continue;
			struct stat st;
			bool is_directory = entry->d_type == DT_DIR ||
			                    (entry->d_type == DT_UNKNOWN &&
			                     !fstatat(dirfd(dir), entry->d_name, &st, 0) &&
			                     S_ISDIR(st.st_mode));
			bool is_executable = complete_type == COMPLETE_TYPE_EXECUTABLE &&
			                     !fstatat(dirfd(dir), entry->d_name, &st, 0) &&
			                     st.st_mode & 0111;
			if ( complete_type == COMPLETE_TYPE_DIRECTORY && !is_directory )
				continue;
			if ( complete_type == COMPLETE_TYPE_EXECUTABLE &&
			     !(is_directory || is_executable) )
				continue;
			size_t name_length = strlen(entry->d_name);
			char* completion = (char*) malloc(name_length - pattern_length + 1 + 1);
			if ( !completion )
				continue;
			strcpy(completion, entry->d_name + pattern_length);
			if ( is_directory )
				strcat(completion, "/");
			completions[num_completions++] = completion;
		}
		closedir(dir);
		free(dirpath_alloc);
	} while ( false );

	if ( complete_type == COMPLETE_TYPE_VARIABLE ) do
	{
		const char* pattern = partial + complete_at - used_before + 1;
		size_t pattern_length = used_before - 1;
		if ( memchr(pattern, '=', pattern_length) )
			break;
		for ( size_t i = 0; environ[i]; i++ )
		{
			if ( strncmp(pattern, environ[i], pattern_length) != 0 )
				continue;
			const char* rest = environ[i] + pattern_length;
			size_t equal_offset = strcspn(rest, "=");
			if ( rest[equal_offset] != '=' )
				continue;
			completions[num_completions++] = strndup(rest, equal_offset);
		}
	} while ( false );

	*used_before_ptr = used_before;
	*used_after_ptr = used_after;

	return *completions_ptr = completions, num_completions;
}

int get_and_run_command_interactive(bool exit_on_error, bool* exitexec)
{
	update_pwd();
	update_env();

	static struct edit_line edit_state;

	edit_state.in_fd = 0;
	edit_state.out_fd = 1;
	edit_state.check_input_incomplete_context = NULL;
	edit_state.check_input_incomplete = does_line_editing_need_another_line;
	edit_state.trap_eof_opportunity_context = &edit_state;
	edit_state.trap_eof_opportunity = on_trap_eof;
	edit_state.complete_context = NULL;
	edit_state.complete = do_complete;

	const char* print_username = getlogin();
	if ( !print_username )
		print_username = getuid() == 0 ? "root" : "?";
	char hostname[256];
	if ( gethostname(hostname, sizeof(hostname)) < 0 )
		strlcpy(hostname, "(none)", sizeof(hostname));
	const char* print_hostname = hostname;
	const char* print_dir = getenv_safe("PWD", "?");
	const char* home_dir = getenv_safe("HOME", "");

	const char* print_dir_1 = print_dir;
	const char* print_dir_2 = "";

	size_t home_dir_len = strlen(home_dir);
	if ( home_dir_len && strncmp(print_dir, home_dir, home_dir_len) == 0 )
	{
		print_dir_1 = "~";
		print_dir_2 = print_dir + home_dir_len;
	}

	char* ps1;
	asprintf(&ps1, "\e[32m%s@%s \e[36m%s%s #\e[37m ",
		print_username,
		print_hostname,
		print_dir_1,
		print_dir_2);

	edit_state.ps1 = ps1;
	edit_state.ps2 = "> ";

	edit_line(&edit_state);

	int result = status;

	if ( edit_state.eof_condition )
	{
		if ( is_outermost_shell() )
			printf("Type exit to close the outermost shell.\n");
		else
			*exitexec = true;
	}
	else if ( !edit_state.abort_editing )
	{
		char* command = edit_line_result(&edit_state);
		for ( size_t i = 0; command[i]; i++ )
			if ( command[i + 0] == '\\' && command[i + 1] == '\n' )
				command[i + 0] = ' ',
				command[i + 1] = ' ';
		result = run_command(command, true, exit_on_error, exitexec);
		free(command);
	}

	free(ps1);

	return result;
}

int get_and_run_command_non_interactive(FILE* fp,
                                        const char* fpname,
                                        bool exit_on_error,
                                        bool* exitexec)
{
	int fd = fileno(fp);

	static char* command = NULL;
	static size_t commandlen = 1024;
	if ( !command )
		commandlen = 1024,
		command = (char*) malloc((commandlen+1) * sizeof(char));
	if ( !command )
		error(64, errno, "malloc");

	command[0] = '\0';

	size_t commandused = 0;
	while ( true )
	{
		char c;
		ssize_t bytesread;
		if ( 0 <= fd )
			bytesread = read(fd, &c, sizeof(c));
		else
		{
			int ic = fgetc(fp);
			if ( ic == EOF && ferror(fp) )
				bytesread = -1;
			else if ( ic == EOF )
				bytesread = 0;
			else
				c = (char) (unsigned char) ic, bytesread = 1;
		}
		if ( bytesread < 0 && errno == EINTR )
			return status;
		if ( bytesread < 0 )
			error(64, errno, "read %s", fpname);
		if ( !bytesread )
		{
			if ( commandused )
				break;
			*exitexec = true;
			return status;
		}
		if ( !c )
			continue;
		if ( c == '\n' && is_shell_input_ready(command) )
			break;
		if ( commandused == commandlen )
		{
			size_t newlen = commandlen * 2;
			size_t newsize = (newlen+1) * sizeof(char);
			char* newcommand = (char*) realloc(command, newsize);
			if ( !newcommand )
				error(64, errno, "realloc");
			command = newcommand;
			commandlen = newsize;
		}
		command[commandused++] = c;
		command[commandused] = '\0';
	}

	return run_command(command, false, exit_on_error, exitexec);
}

void load_argv_variables(int argc, char* argv[])
{
	char varname[sizeof(int) * 3];
	for ( int i = 0; i < argc; i++ )
		snprintf(varname, sizeof(varname), "%i", i),
		setenv(varname, argv[i], 1);
}

int run(FILE* fp, int argc, char* argv[], const char* name, bool interactive,
        bool exit_on_error)
{
	load_argv_variables(argc, argv);
	bool exitexec = false;
	int exitstatus;
	do
	{
		if ( interactive )
			exitstatus = get_and_run_command_interactive(exit_on_error, &exitexec);
		else
			exitstatus =
				get_and_run_command_non_interactive(fp, name, exit_on_error, &exitexec);
	}
	while ( !exitexec );
	return exitstatus;
}

int run_interactive(int argc, char* argv[], bool exit_on_error)
{
	return run(stdin, argc, argv, "<stdin>", true, exit_on_error);
}

int run_stdin(int argc, char* argv[], bool exit_on_error)
{
	return run(stdin, argc, argv, "<stdin>", false, exit_on_error);
}

int run_string(int argc, char* argv[], const char* str, bool exit_on_error)
{
	FILE* fp = fmemopen((void*) str, strlen(str), "r");
	if ( !fp ) { error(0, errno, "fmemopen"); return 1; }
	int ret = run(fp, argc, argv, "<command-line>", false, exit_on_error);
	fclose(fp);
	return ret;
}

int run_script(const char* path, int argc, char* argv[], bool exit_on_error)
{
	FILE* fp = fopen(path, "r");
	if ( !fp ) { error(0, errno, "%s", path); return 127; }
	int ret = run(fp, argc, argv, path, false, exit_on_error);
	fclose(fp);
	return ret;
}

int main(int argc, char* argv[])
{
	bool exit_on_error = false;
	if ( 2 <= argc && !strcmp(argv[1], "-e") )
	{
		exit_on_error = true;
		argc--;
		for ( int i = 1; i < argc; i++ )
			argv[i] = argv[i+1];
		argv[argc] = NULL;
	}
	if ( getenv("SHLVL") )
	{
		long shlvl = atol(getenv("SHLVL"));
		if ( shlvl < 1 )
			shlvl = 1;
		else
			shlvl++;
		char shlvl_string[sizeof(long) * 3];
		snprintf(shlvl_string, sizeof(shlvl_string), "%li", shlvl);
		setenv("SHLVL", shlvl_string, 1);
	}
	else
	{
		setenv("SHLVL", "1", 1);
	}
	char pidstr[3 * sizeof(pid_t)];
	char ppidstr[3 * sizeof(pid_t)];
	snprintf(pidstr, sizeof(pidstr), "%ji", (intmax_t) getpid());
	snprintf(ppidstr, sizeof(ppidstr), "%ji", (intmax_t) getppid());
	setenv("SHELL", argv[0], 1);
	setenv("$", pidstr, 1);
	setenv("PPID", ppidstr, 1);
	setenv("?", "0", 1);
	update_pwd();
	if ( 2 <= argc && !strcmp(argv[1], "-c") )
		return run_string(argc, argv, argv[2], exit_on_error);
	if ( 1 < argc )
		return run_script(argv[1], argc-1, argv+1, exit_on_error);
	if ( isatty(0) )
		return run_interactive(argc, argv, exit_on_error);
	return run_stdin(argc, argv, exit_on_error);
}

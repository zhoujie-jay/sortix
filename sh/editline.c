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
 * editline.c
 * Read a line from the terminal.
 */

#include <sys/keycodes.h>
#include <sys/termmode.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "editline.h"
#include "showline.h"

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

	if ( wc == L'\b' || wc == 127 )
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

		int kbkey;
		if ( (kbkey = KBKEY_DECODE(codepoint)) )
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

	settermmode(edit_state->in_fd, TERMMODE_NORMAL);
}

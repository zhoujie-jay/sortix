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
 * editline.h
 * Read a line from the terminal.
 */

#ifndef EDITLINE_H
#define EDITLINE_H

#include <stddef.h>

#include "showline.h"

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

void edit_line_show(struct edit_line* edit_state);
char* edit_line_result(struct edit_line* edit_state);
bool edit_line_can_finish(struct edit_line* edit_state);
void edit_line_append_history(struct edit_line* edit_state, const char* line);
void edit_line_type_use_record(struct edit_line* edit_state, const char* record);
void edit_line_type_history_save_at(struct edit_line* edit_state, size_t index);
void edit_line_type_history_save_current(struct edit_line* edit_state);
void edit_line_type_history_prev(struct edit_line* edit_state);
void edit_line_type_history_next(struct edit_line* edit_state);
void edit_line_type_codepoint(struct edit_line* edit_state, wchar_t wc);
void line_edit_type_home(struct edit_line* edit_state);
void line_edit_type_left(struct edit_line* edit_state);
void line_edit_type_right(struct edit_line* edit_state);
void line_edit_type_end(struct edit_line* edit_state);
void line_edit_type_backspace(struct edit_line* edit_state);
void line_edit_type_previous_word(struct edit_line* edit_state);
void line_edit_type_next_word(struct edit_line* edit_state);
void line_edit_type_delete(struct edit_line* edit_state);
void line_edit_type_eof_or_delete(struct edit_line* edit_state);
void edit_line_type_interrupt(struct edit_line* edit_state);
void edit_line_type_kill_after(struct edit_line* edit_state);
void edit_line_type_kill_before(struct edit_line* edit_state);
void edit_line_type_clear(struct edit_line* edit_state);
void edit_line_type_delete_word_before(struct edit_line* edit_state);
int edit_line_completion_sort(const void* a_ptr, const void* b_ptr);
void edit_line_type_complete(struct edit_line* edit_state);
void edit_line_kbkey(struct edit_line* edit_state, int kbkey);
void edit_line_codepoint(struct edit_line* edit_state, wchar_t wc);
void edit_line(struct edit_line* edit_state);

#endif

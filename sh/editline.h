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

    editline.h
    Read a line from the terminal.

*******************************************************************************/

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

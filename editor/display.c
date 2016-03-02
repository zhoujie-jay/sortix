/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * display.c
 * Display handling.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "display.h"
#include "editor.h"
#include "multibyte.h"
#include "terminal.h"

size_t editor_display_column_of_line_offset(struct editor* editor,
                                            const struct line* line,
                                            size_t offset)
{
	if ( line->used < offset )
		offset = line->used;
	return displayed_string_length(line->data, offset, editor->tabsize);
}

size_t editor_line_offset_of_display_column(struct editor* editor,
                                            const struct line* line,
                                            size_t column)
{
	size_t current_column = 0;
	for ( size_t offset = 0; offset < line->used; offset++ )
	{
		if ( column <= current_column )
			return offset;

		wchar_t wc = line->data[offset];

		if ( wc == L'\t' )
		{
			size_t old_column = current_column;
			do current_column++;
			while ( current_column % editor->tabsize != 0 );
			if ( column <= current_column )
			{
				size_t dist_to_old = column - old_column;
				size_t dist_to_cur = current_column - column;
				if ( dist_to_old < dist_to_cur )
					return offset;
				return offset + 1;
			}
			continue;
		}

		current_column++;
	}
	return line->used;
}

size_t displayed_string_length(const wchar_t* str, size_t len, size_t tabsize)
{
	size_t ret_len = 0;
	for ( size_t i = 0; i < len; i++ )
		if ( str[i] == L'\t' )
			do ret_len++;
			while ( ret_len % tabsize );
		else
			ret_len++;
	return ret_len;
}

struct display_char* expand_tabs(const wchar_t* str, size_t len, uint8_t* colors,
                                 size_t colors_len, size_t* ret_len_ptr,
                                 size_t tabsize)
{
	size_t ret_len = displayed_string_length(str, len, tabsize);
	struct display_char* ret = (struct display_char*)
		malloc(sizeof(struct display_char) * (ret_len + 1));
	for ( size_t i = 0, j = 0; i < len; i++ )
	{
		uint8_t color = i < colors_len ? colors[i] : 7;
		if ( str[i] == L'\t' )
			do ret[j++] = (struct display_char) { L' ', color};
			while ( j % tabsize );
		else
			ret[j++] = (struct display_char) { str[i], color };
	}
	ret[ret_len] = (struct display_char) { L'\0', 0 };
	if ( ret_len_ptr )
		*ret_len_ptr = ret_len;
	return ret;
}

void render_editor(struct editor* editor, struct terminal_state* state)
{
	if ( state->height < 1 )
		return;

	// Create the header title bar.
	for ( int x = 0; x < state->width; x++ )
		state->data[0 * state->width + x] = make_terminal_datum(L' ', 0x70);

	// Render the name of the program.
	const wchar_t* header_start = editor->dirty ? L"  editor          *"
	                                            : L"  editor           ";
	size_t header_start_len = wcslen(header_start);

	for ( size_t i = 0; i < header_start_len; i++ )
		if ( i < (size_t) state->width)
			state->data[i].character = header_start[i];

	// Render the name of the currently open file.
	const char* file_name = editor->current_file_name;
	if ( !file_name )
		file_name = "New File";

	wchar_t* wcs_file_name = convert_mbs_to_wcs(file_name);
	size_t wcs_file_name_len = wcslen(wcs_file_name);
	for ( size_t i = 0; i < wcs_file_name_len; i++ )
		if ( header_start_len+i < (size_t) state->width)
			state->data[header_start_len+i].character = wcs_file_name[i];
	free(wcs_file_name);

	size_t line_number_width = 1;
	for ( size_t tmp = editor->lines_used; 10 <= tmp; tmp /= 10 )
		line_number_width++;
	if ( !editor->line_numbering )
		line_number_width = 0;

	// Calculate the dimensions of the viewport.
	size_t viewport_top = 1;
	size_t viewport_left = line_number_width;

	editor->viewport_width = (size_t) state->width;
	if ( editor->viewport_width < viewport_left )
		editor->viewport_width = 0;
	else
		editor->viewport_width -= viewport_left;

	editor->viewport_height = (size_t) state->height;
	if ( editor->viewport_height < viewport_top )
		editor->viewport_height = 0;
	else
		editor->viewport_height -= viewport_top;

	if ( !editor->viewport_height )
		return;

	// Decide which page of the file to render and the cursor position on it.
	struct line* current_line = &editor->lines[editor->cursor_row];
	size_t cursor_x = displayed_string_length(current_line->data,
	                                          editor->cursor_column,
	                                          editor->tabsize);
	size_t cursor_y = editor->cursor_row;
	struct line* select_line = &editor->lines[editor->select_row];
	size_t select_x = displayed_string_length(select_line->data,
	                                          editor->select_column,
	                                          editor->tabsize);
	size_t select_y = editor->select_row;

	size_t page_x_offset = editor->page_x_offset;
	size_t page_y_offset = editor->page_y_offset;

	bool has_selection = !(editor->cursor_row == editor->select_row &&
	                       editor->cursor_column == editor->select_column);
	size_t viewport_select_x = select_x - page_x_offset;
	size_t viewport_select_y = select_y - page_y_offset;

	// Render this page of text.
	for ( size_t y = 0; y < editor->viewport_height; y++ )
	{
		size_t line_index = page_y_offset + y;
		struct terminal_datum* raw_data_line =
			state->data + (viewport_top + y) * state->width;
		struct terminal_datum* data_line = raw_data_line + viewport_left;
		struct line* line = line_index < editor->lines_used ?
		                    &editor->lines[line_index] : NULL;
		struct color_line* color_line = line_index < editor->color_lines_used ?
		                                &editor->color_lines[line_index] : NULL;
		size_t expanded_len;
		struct display_char* expanded
			= expand_tabs(line ? line->data : L"",
			              line ? line->used : 0,
			              color_line ? color_line->data : NULL,
			              color_line ? color_line->length : 0,
			              &expanded_len,
			              editor->tabsize);
		const struct display_char* chars = expanded;
		size_t chars_length = expanded_len;
		if ( chars_length < page_x_offset )
			chars = NULL, chars_length = 0;
		else
			chars += page_x_offset, chars_length -= page_x_offset;
		for ( size_t x = 0; x < line_number_width; x++ )
			raw_data_line[x] = make_terminal_datum(L' ', 0x70);
		if ( editor->line_numbering && line_index < editor->lines_used )
		{
			char line_number[sizeof(size_t) * 3];
			snprintf(line_number, sizeof(line_number), "%zu", line_index + 1);
			size_t length = strlen(line_number);
			size_t offset = line_number_width - length;
			for ( size_t i = 0; i < length; i++ )
				raw_data_line[offset + i].character = btowc(line_number[i]);
		}
		for ( size_t x = 0; x < editor->viewport_width; x++ )
		{
			size_t column_index = page_x_offset + x;
			bool selected = (is_row_column_lt(cursor_y, cursor_x, select_y, select_x) &&
			                 is_row_column_le(cursor_y, cursor_x, line_index, column_index) &&
			                 is_row_column_lt(line_index, column_index, select_y, select_x)) ||
                            (is_row_column_lt(select_y, select_x, cursor_y, cursor_x) &&
			                 is_row_column_le(select_y, select_x, line_index, column_index) &&
			                 is_row_column_lt(line_index, column_index, cursor_y, cursor_x));
			bool at_margin = column_index == editor->margin;
			bool is_blank = chars_length <= x;
			wchar_t c = is_blank ? L' ' : chars[x].character;
			uint8_t color = (is_blank ? 7 : chars[x].color);
			data_line[x] = selected && is_blank && at_margin ? make_terminal_datum(L'|', 0x41) :
                           selected ? make_terminal_datum(c, 0x47) :
			               is_blank && at_margin ? make_terminal_datum(L'|', 0x01) :
			               make_terminal_datum(c, color);
		}
		free(expanded);
	}

	// Set the rest of the terminal state.
	if ( has_selection )
	{
		state->cursor_x = state->width;
		state->cursor_y = state->height;
	}
	else
	{
		state->cursor_x = viewport_left + viewport_select_x;
		state->cursor_y = viewport_top + viewport_select_y;
	}
	state->color = 0x07;

	if ( editor->mode == MODE_EDIT )
		return;

	const char* msg = "";
	if ( editor->mode == MODE_SAVE ||
	     editor->mode == MODE_SAVE_LOAD ||
	     editor->mode == MODE_SAVE_QUIT )
		msg = "File Name to Write: ";
	if ( editor->mode == MODE_LOAD )
		msg = "File Name to Read: ";
	if ( editor->mode == MODE_ASK_LOAD )
		msg = "Save modified file? (Y/N): ";
	if ( editor->mode == MODE_ASK_QUIT )
		msg = "Save modified file? (Y/N): ";
	if ( editor->mode == MODE_GOTO_LINE )
		msg = "Go to line: ";
	if ( editor->mode == MODE_COMMAND )
		msg = "Enter miscellaneous command: ";

	struct terminal_datum* data_line = state->data + (state->height - 1) * state->width;
	wchar_t* wcs_msg = convert_mbs_to_wcs(msg);
	size_t wcs_msg_len = wcslen(wcs_msg);
	for ( size_t i = 0; i < wcs_msg_len; i++ )
		if ( i < (size_t) state->width)
			data_line[i] = make_terminal_datum(wcs_msg[i], 0x70);
	free(wcs_msg);

	if ( (size_t) state->width <= wcs_msg_len )
		return;

	size_t modal_viewport_width = state->width - wcs_msg_len;
	size_t modal_viewport_cursor = editor->modal_cursor % modal_viewport_width;
	size_t modal_viewport_page = editor->modal_cursor / modal_viewport_width;
	size_t modal_viewport_offset = modal_viewport_page * modal_viewport_width;

	struct terminal_datum* modal_viewport_data = data_line + wcs_msg_len;

	const wchar_t* modal_chars = editor->modal;
	size_t modal_chars_length = editor->modal_used;
	if ( modal_chars_length < modal_viewport_offset )
		modal_chars = NULL,
		modal_chars_length = 0;
	else
		modal_chars += modal_viewport_offset,
		modal_chars_length -= modal_viewport_offset;

	for ( size_t x = 0; x < modal_viewport_width; x++ )
	{
		wchar_t c = x < modal_chars_length ? modal_chars[x] : L' ';
		uint16_t color = editor->modal_error ? 0x17 : 0x70;
		uint16_t tab_color = editor->modal_error ? 0x12 : 0x71;
		if ( c == L'\t' )
			modal_viewport_data[x] = make_terminal_datum(L'>', tab_color);
		else
			modal_viewport_data[x] = make_terminal_datum(c, color);
	}

	state->cursor_x = wcs_msg_len + modal_viewport_cursor;
	state->cursor_y = state->height - 1;
	state->color = 0x70;
}

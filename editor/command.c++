/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    command.c++
    Editor commands.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "command.h++"
#include "cursor.h++"
#include "display.h++"
#include "editor.h++"
#include "multibyte.h++"
#include "terminal.h++"

void editor_type_newline(struct editor* editor)
{
	editor->dirty = true;

	if ( editor->lines_used == editor->lines_length )
	{
		size_t new_length = editor->lines_length ? 2 * editor->lines_length : 8;
		struct line* new_lines = new struct line[new_length];
		for ( size_t i = 0; i < editor->lines_used; i++ )
			new_lines[i] = editor->lines[i];
		delete[] editor->lines;
		editor->lines = new_lines;
		editor->lines_length = new_length;
	}

	for ( size_t i = editor->lines_used-1; editor->cursor_row < i; i-- )
		editor->lines[i+1] = editor->lines[i];
	editor->lines_used++;

	struct line old_line = editor->lines[editor->cursor_row];

	size_t keep_length = editor->cursor_column;
	size_t move_length = old_line.used - keep_length;

	struct line* keep_line = &editor->lines[editor->cursor_row];
	struct line* move_line = &editor->lines[editor->cursor_row+1];

	keep_line->data = new wchar_t[keep_length];
	keep_line->used = keep_length;
	keep_line->length = keep_length;
	memcpy(keep_line->data, old_line.data + 0, sizeof(wchar_t) * keep_length);

	move_line->data = new wchar_t[move_length];
	move_line->used = move_length;
	move_line->length = move_length;
	memcpy(move_line->data, old_line.data + keep_length, sizeof(wchar_t) * move_length);

	editor_cursor_set(editor, editor->cursor_row+1, 0);

	delete[] old_line.data;
}

void editor_type_delete_selection(struct editor* editor);

void editor_type_combine_with_last(struct editor* editor)
{
	if ( !editor->cursor_row )
		return;

	editor->dirty = true;

	struct line* keep_line = &editor->lines[editor->cursor_row-1];
	struct line* gone_line = &editor->lines[editor->cursor_row];

	wchar_t* keep_line_data = keep_line->data;
	wchar_t* gone_line_data = gone_line->data;

	size_t new_length = keep_line->used + gone_line->used;
	wchar_t* new_data = new wchar_t[new_length];

	memcpy(new_data, keep_line_data, sizeof(wchar_t) * keep_line->used);
	memcpy(new_data + keep_line->used, gone_line_data, sizeof(wchar_t) * gone_line->used);

	editor_cursor_set(editor, editor->cursor_row-1, keep_line->used);

	keep_line->data = new_data;
	keep_line->used = new_length;
	keep_line->length = new_length;

	editor->lines_used--;
	for ( size_t i = editor->cursor_row + 1; i < editor->lines_used; i++ )
		editor->lines[i] = editor->lines[i+1];

	free(keep_line_data);
	free(gone_line_data);
}

void editor_type_backspace(struct editor* editor)
{
	if ( !(editor->select_row == editor->cursor_row &&
	       editor->select_column == editor->cursor_column) )
	{
		editor_type_delete_selection(editor);
		return;
	}

	struct line* current_line = &editor->lines[editor->cursor_row];

	if ( !editor->cursor_column )
	{
		editor_type_combine_with_last(editor);
		return;
	}

	editor->dirty = true;

	current_line->used--;
	for ( size_t i = editor_cursor_column_dec(editor); i < current_line->used; i++ )
		current_line->data[i] = current_line->data[i+1];
}

void editor_type_combine_with_next(struct editor* editor)
{
	if ( editor->cursor_row + 1 == editor->lines_used )
		return;

	editor->dirty = true;

	struct line* keep_line = &editor->lines[editor->cursor_row];
	struct line* gone_line = &editor->lines[editor->cursor_row+1];

	wchar_t* keep_line_data = keep_line->data;
	wchar_t* gone_line_data = gone_line->data;

	size_t new_length = keep_line->used + gone_line->used;
	wchar_t* new_data = new wchar_t[new_length];

	memcpy(new_data, keep_line_data, sizeof(wchar_t) * keep_line->used);
	memcpy(new_data + keep_line->used, gone_line_data, sizeof(wchar_t) * gone_line->used);

	editor_cursor_column_set(editor, keep_line->used);

	keep_line->data = new_data;
	keep_line->used = new_length;
	keep_line->length = new_length;

	editor->lines_used--;
	for ( size_t i = editor->cursor_row + 1; i < editor->lines_used; i++ )
		editor->lines[i] = editor->lines[i+1];

	free(keep_line_data);
	free(gone_line_data);
}

void editor_type_delete(struct editor* editor)
{
	if ( !(editor->select_row == editor->cursor_row &&
	       editor->select_column == editor->cursor_column) )
	{
		editor_type_delete_selection(editor);
		return;
	}

	struct line* current_line = &editor->lines[editor->cursor_row];

	if ( editor->cursor_column == current_line->used )
	{
		editor_type_combine_with_next(editor);
		return;
	}

	editor->dirty = true;

	current_line->used--;
	for ( size_t i = editor->cursor_column; i < current_line->used; i++ )
		current_line->data[i] = current_line->data[i+1];
}

void editor_type_delete_selection(struct editor* editor)
{
	if ( is_row_column_lt(editor->select_row, editor->select_column,
	                      editor->cursor_row, editor->cursor_column) )
	{
		size_t tmp;
		tmp = editor->select_row;
		editor->select_row = editor->cursor_row;
		editor->cursor_row = tmp;
		tmp = editor->select_column;
		editor->select_column = editor->cursor_column;
		editor->cursor_column = tmp;
	}

	size_t desired_row = editor->cursor_row;
	size_t desired_column = editor->cursor_column;

	editor->cursor_row = editor->select_row;
	editor->cursor_column = editor->select_column;

	while ( !(editor->cursor_row == desired_row &&
	          editor->cursor_column == desired_column) )
		editor_type_backspace(editor);
}

void editor_type_left(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
		return;
	}
	if ( editor->cursor_column )
		editor_cursor_column_dec(editor);
	else if ( editor->cursor_row )
	{
		editor_cursor_row_dec(editor);
		editor_cursor_column_set(editor, editor->lines[editor->cursor_row].used);
	}
}

void editor_type_select_left(struct editor* editor)
{
	if ( editor->select_column )
		editor_select_column_dec(editor);
	else if ( editor->select_row )
	{
		editor_select_row_dec(editor);
		editor_select_column_set(editor, editor->lines[editor->select_row].used);
	}
}

void editor_type_right(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                  editor->select_row, editor->select_column,
		                  &column, &row);
		editor_cursor_set(editor, column, row);
		return;
	}
	struct line* current_line = &editor->lines[editor->cursor_row];
	if ( editor->cursor_column != current_line->used )
		editor_cursor_column_inc(editor);
	else if ( editor->cursor_row+1 != editor->lines_used )
		editor_cursor_row_inc(editor),
		editor_cursor_column_set(editor, 0);
}

void editor_type_select_right(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	if ( editor->select_column != current_line->used )
		editor_select_column_inc(editor);
	else if ( editor->select_row+1 != editor->lines_used )
		editor_select_row_inc(editor),
		editor_select_column_set(editor, 0);
}

void editor_type_up(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( !editor->cursor_row )
	{
		editor_cursor_column_set(editor, 0);
		return;
	}
	size_t new_line_len = editor->lines[editor_cursor_row_dec(editor)].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_up(struct editor* editor)
{
	if ( !editor->select_row )
	{
		editor_select_column_set(editor, 0);
		return;
	}
	size_t new_line_len = editor->lines[editor_select_row_dec(editor)].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_type_down(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                   editor->select_row, editor->select_column,
		                   &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( editor->cursor_row+1 == editor->lines_used  )
	{
		editor_cursor_column_set(editor, editor->lines[editor->cursor_row].used);
		return;
	}
	size_t new_line_len = editor->lines[editor_cursor_row_inc(editor)].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_down(struct editor* editor)
{
	if ( editor->select_row+1 == editor->lines_used  )
	{
		editor_select_column_set(editor, editor->lines[editor->select_row].used);
		return;
	}
	size_t new_line_len = editor->lines[editor_select_row_inc(editor)].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_skip_leading(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->cursor_row];
	for ( editor_cursor_column_set(editor, 0);
	      editor->cursor_column < current_line->used;
	      editor_cursor_column_inc(editor) )
		if ( !iswspace(current_line->data[editor->cursor_column]) )
			break;
}

void editor_select_skip_leading(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	for ( editor_select_column_set(editor, 0);
	      editor->select_column < current_line->used;
	      editor_select_column_inc(editor) )
		if ( !iswspace(current_line->data[editor->select_column]) )
			break;
}

void editor_type_home(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( !editor->cursor_column )
	{
		editor_skip_leading(editor);
		return;
	}
	editor_cursor_column_set(editor, 0);
}

void editor_type_select_home(struct editor* editor)
{
	if ( !editor->select_column )
	{
		editor_select_skip_leading(editor);
		return;
	}
	editor_select_column_set(editor, 0);
}

void editor_skip_ending(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->cursor_row];
	for ( editor_cursor_column_set(editor, current_line->used);
	      editor->cursor_column;
	      editor_cursor_column_dec(editor) )
		if ( !iswspace(current_line->data[editor->cursor_column-1]) )
			break;
}

void editor_select_skip_ending(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	for ( editor_select_column_set(editor, current_line->used);
	      editor->select_column;
	      editor_select_column_dec(editor) )
		if ( !iswspace(current_line->data[editor->select_column-1]) )
			break;
}

void editor_type_end(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                   editor->select_row, editor->select_column,
		                   &column, &row);
		editor_cursor_set(editor, column, row);
	}
	struct line* current_line = &editor->lines[editor->cursor_row];
	if ( editor->cursor_column == current_line->used )
	{
		editor_skip_ending(editor);
		return;
	}
	editor_cursor_column_set(editor, current_line->used);
}

void editor_type_select_end(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	if ( editor->select_column == current_line->used )
	{
		editor_select_skip_ending(editor);
		return;
	}
	editor_select_column_set(editor, current_line->used);
}

void editor_type_page_up(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( editor->cursor_row < editor->viewport_height )
	{
		editor_cursor_set(editor, 0, 0);
		return;
	}
	size_t new_line = editor->cursor_row - editor->viewport_height;
	editor_cursor_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_page_up(struct editor* editor)
{
	if ( editor->select_row < editor->viewport_height )
	{
		editor_select_set(editor, 0, 0);
		return;
	}
	size_t new_line = editor->select_row - editor->viewport_height;
	editor_select_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_type_page_down(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                   editor->select_row, editor->select_column,
		                   &column, &row);
		editor_cursor_set(editor, column, row);
	}
	size_t new_line = editor->cursor_row + editor->viewport_height;
	if ( editor->lines_used <= new_line )
	{
		editor_cursor_row_set(editor, editor->lines_used - 1);
		editor_cursor_column_set(editor, editor->lines[editor->cursor_row].used);
		return;
	}
	editor_cursor_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_page_down(struct editor* editor)
{
	size_t new_line = editor->select_row + editor->viewport_height;
	if ( editor->lines_used <= new_line )
	{
		editor_select_row_set(editor, editor->lines_used - 1);
		editor_select_column_set(editor, editor->lines[editor->select_row].used);
		return;
	}
	editor_select_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_type_edit(struct editor* editor)
{
	editor->mode = MODE_EDIT;
}

void editor_type_goto_line(struct editor* editor)
{
	editor->mode = MODE_GOTO_LINE;
	editor->modal_used = 0;
	editor->modal_cursor = 0;
	editor->modal_error = false;
}

void editor_type_save(struct editor* editor)
{
	editor->mode = MODE_SAVE;

	free(editor->modal);
	editor->modal = convert_mbs_to_wcs(editor->current_file_name);
	editor->modal_used = wcslen(editor->modal);
	editor->modal_length = editor->modal_used+1;
	editor->modal_cursor = editor->modal_used;
	editor->modal_error = false;
}

void editor_type_save_as(struct editor* editor)
{
	editor->mode = MODE_SAVE;
	editor->modal_used = 0;
	editor->modal_cursor = 0;
	editor->modal_error = false;
}

void editor_type_open(struct editor* editor)
{
	editor->mode = MODE_LOAD;
	editor->modal_used = 0;
	editor->modal_cursor = 0;
	editor->modal_error = false;
}

void editor_type_open_as(struct editor* editor)
{
	editor->mode = MODE_LOAD;

	free(editor->modal);
	editor->modal = convert_mbs_to_wcs(editor->current_file_name);
	editor->modal_used = wcslen(editor->modal);
	editor->modal_length = editor->modal_used+1;
	editor->modal_cursor = editor->modal_used;
	editor->modal_error = false;
}

void editor_type_quit(struct editor* editor)
{
	editor->mode = editor->dirty ? MODE_ASK_QUIT : MODE_QUIT;
	editor->modal_cursor = 0;
	editor->modal_used = 0;
	editor->modal_error = false;
}

void editor_type_command(struct editor* editor)
{
	editor->mode = MODE_COMMAND;
	editor->modal_cursor = 0;
	editor->modal_used = 0;
	editor->modal_error = false;
}

void editor_type_raw_character(struct editor* editor, wchar_t c)
{
	struct line* current_line = &editor->lines[editor->cursor_row];

	if ( current_line->used == current_line->length )
	{
		size_t new_length = current_line->length ? 2 * current_line->length : 8;
		wchar_t* new_data = new wchar_t[new_length];
		for ( size_t i = 0; i < current_line->used; i++ )
			new_data[i] = current_line->data[i];
		delete[] current_line->data;
		current_line->data = new_data;
		current_line->length = new_length;
	}

	editor->dirty = true;

	for ( size_t i = current_line->used; editor->cursor_column < i; i-- )
		current_line->data[i] = current_line->data[i-1];
	current_line->used++;
	current_line->data[editor_cursor_column_inc(editor)-1] = c;
}

void editor_type_copy(struct editor* editor)
{
	if ( editor->cursor_row == editor->select_row &&
	     editor->cursor_column == editor->select_column )
		return;

	delete[] editor->clipboard;

	size_t start_row;
	size_t start_column;
	size_t end_row;
	size_t end_column;
	if ( is_row_column_lt(editor->select_row, editor->select_column,
	                      editor->cursor_row, editor->cursor_column) )
	{
		start_row = editor->select_row;
		start_column = editor->select_column;
		end_row = editor->cursor_row;
		end_column = editor->cursor_column;
	}
	else
	{
		start_row = editor->cursor_row;
		start_column = editor->cursor_column;
		end_row = editor->select_row;
		end_column = editor->select_column;
	}

	size_t length = 0;
	for ( size_t row = start_row, column = start_column;
	      is_row_column_lt(row, column, end_row, end_column); )
	{
		if ( row == end_row )
		{
			length += end_column - column;
			column = end_column;
		}
		else
		{
			length += editor->lines[row].used + 1 /*newline*/;
			column = 0;
			row++;
		}
	}

	editor->clipboard = new wchar_t[length + 1];
	size_t offset = 0;
	for ( size_t row = start_row, column = start_column;
	      is_row_column_lt(row, column, end_row, end_column); )
	{
		struct line* line = &editor->lines[row];
		if ( row == end_row )
		{
			memcpy(editor->clipboard + offset, line->data + column, sizeof(wchar_t) * (end_column - column));
			offset += end_column - column;
			column = end_column;
		}
		else
		{
			memcpy(editor->clipboard + offset, line->data, sizeof(wchar_t) * line->used);
			editor->clipboard[offset + line->used] = L'\n';
			offset += line->used + 1 /*newline*/;
			column = 0;
			row++;
		}
	}
	editor->clipboard[length] = L'\0';
}

void editor_type_cut(struct editor* editor)
{
	if ( editor->cursor_row == editor->select_row &&
	     editor->cursor_column == editor->select_column )
		return;

	editor_type_copy(editor);
	editor_type_delete_selection(editor);
}

void editor_type_paste(struct editor* editor)
{
	if ( !(editor->cursor_row == editor->select_row &&
	       editor->cursor_column == editor->select_column) )
		editor_type_delete_selection(editor);

	for ( size_t i = 0; editor->clipboard && editor->clipboard[i]; i++ )
	{
		if ( editor->clipboard[i] == L'\n' )
			editor_type_newline(editor);
		else
			editor_type_raw_character(editor, editor->clipboard[i]);
	}
}

void editor_type_character(struct editor* editor, wchar_t c)
{
	if ( editor->control )
	{
		switch ( towlower(c) )
		{
		case L'c': editor_type_copy(editor); break;
		case L'i': editor_type_goto_line(editor); break;
		case L'k': editor_type_cut(editor); break;
		case L'o': editor->shift ?
		           editor_type_open_as(editor) :
		           editor_type_open(editor); break;
		case L'q': editor_type_quit(editor); break;
		case L's': editor->shift ?
		           editor_type_save_as(editor) :
		           editor_type_save(editor); break;
		case L'v': editor_type_paste(editor); break;
		case L'x': editor_type_cut(editor); break;
		}
		return;
	}

	if ( editor_has_selection(editor) )
		editor_type_delete_selection(editor);

	if ( c == L'\n' ) { editor_type_newline(editor); return; }

	editor_type_raw_character(editor, c);
}

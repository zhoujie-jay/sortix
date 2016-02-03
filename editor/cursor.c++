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

    cursor.c++
    Editor cursor.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <assert.h>
#include <stddef.h>

#include "cursor.h++"
#include "display.h++"
#include "editor.h++"

size_t editor_select_column_set(struct editor* editor, size_t x)
{
	if ( editor->viewport_width )
	{
		struct line* line = &editor->lines[editor->select_row];
		size_t rx = displayed_string_length(line->data, x, editor->tabsize);
		if ( rx < editor->page_x_offset )
			editor->page_x_offset = rx;
		if ( editor->page_x_offset + editor->viewport_width <= rx )
			editor->page_x_offset = rx + 1- editor->viewport_width;
	}
	return editor->select_column = x;
}

size_t editor_select_row_set(struct editor* editor, size_t y)
{
	if ( editor->viewport_height )
	{
		if ( y < editor->page_y_offset )
			editor->page_y_offset = y;
		if ( editor->page_y_offset + editor->viewport_height <= y )
			editor->page_y_offset = y + 1- editor->viewport_height;
	}
	return editor->select_row = y;
}

void editor_select_set(struct editor* editor, size_t y, size_t x)
{
	editor_select_column_set(editor, x);
	editor_select_row_set(editor, y);
}

size_t editor_select_column_dec(struct editor* editor)
{
	assert(editor->select_column);
	return editor_select_column_set(editor, editor->select_column-1);
}

size_t editor_select_column_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_select_column_set(editor, editor->select_column+1);
}

size_t editor_select_row_dec(struct editor* editor)
{
	assert(editor->select_row);
	return editor_select_row_set(editor, editor->select_row-1);
}

size_t editor_select_row_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_select_row_set(editor, editor->select_row+1);
}

size_t editor_cursor_column_set(struct editor* editor, size_t x)
{
	editor_select_column_set(editor, x);
	editor_select_row_set(editor, editor->cursor_row);
	return editor->cursor_column = x;
}

size_t editor_cursor_row_set(struct editor* editor, size_t y)
{
	editor_select_column_set(editor, editor->cursor_column);
	editor_select_row_set(editor, y);
	return editor->cursor_row = y;
}

void editor_cursor_set(struct editor* editor, size_t y, size_t x)
{
	editor_cursor_column_set(editor, x);
	editor_cursor_row_set(editor, y);
}

size_t editor_cursor_column_dec(struct editor* editor)
{
	assert(editor->cursor_column);
	return editor_cursor_column_set(editor, editor->cursor_column-1);
}

size_t editor_cursor_column_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_cursor_column_set(editor, editor->cursor_column+1);
}

size_t editor_cursor_row_dec(struct editor* editor)
{
	assert(editor->cursor_row);
	return editor_cursor_row_set(editor, editor->cursor_row-1);
}

size_t editor_cursor_row_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_cursor_row_set(editor, editor->cursor_row+1);
}

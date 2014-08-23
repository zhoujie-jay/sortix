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

    editor.h++
    Editor.

*******************************************************************************/

#ifndef EDITOR_EDITOR_HXX
#define EDITOR_EDITOR_HXX

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct line
{
	wchar_t* data;
	size_t used;
	size_t length;
};

struct color_line
{
	uint8_t* data;
	size_t length;
};

enum editor_mode
{
	MODE_QUIT,
	MODE_EDIT,
	MODE_LOAD,
	MODE_SAVE,
	MODE_ASK_QUIT,
	MODE_GOTO_LINE,
	MODE_COMMAND,
};

struct editor
{
	char* current_file_name;
	struct line* lines;
	size_t lines_used;
	size_t lines_length;
	struct color_line* color_lines;
	size_t color_lines_used;
	size_t color_lines_length;
	size_t cursor_column;
	size_t cursor_row;
	size_t select_column;
	size_t select_row;
	size_t viewport_width;
	size_t viewport_height;
	size_t page_x_offset;
	size_t page_y_offset;
	wchar_t* modal;
	size_t modal_used;
	size_t modal_length;
	size_t modal_cursor;
	wchar_t* clipboard;
	size_t tabsize;
	size_t margin;
	enum editor_mode mode;
	bool control;
	bool shift;
	bool lshift;
	bool rshift;
	bool dirty;
	bool modal_error;
	bool highlight_source;
};

__attribute__((unused))
static inline bool editor_has_selection(struct editor* editor)
{
	return !(editor->cursor_row == editor->select_row &&
	         editor->cursor_column == editor->select_column);
}

void initialize_editor(struct editor* editor);
void editor_reset_contents(struct editor* editor);
bool editor_load_file_contents(struct editor* editor, FILE* fp);
bool editor_load_file(struct editor* editor, const char* path);
bool editor_load_popen(struct editor* editor, const char* cmd);
bool editor_save_file(struct editor* editor, const char* path);

#endif

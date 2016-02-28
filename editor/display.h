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

    display.h
    Display handling.

*******************************************************************************/

#ifndef EDITOR_DISPLAY_H
#define EDITOR_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

struct editor;
struct line;
struct terminal_state;

struct display_char
{
	wchar_t character;
	uint8_t color;
};

size_t editor_display_column_of_line_offset(struct editor* editor,
                                            const struct line* line,
                                            size_t offset);
size_t editor_line_offset_of_display_column(struct editor* editor,
                                            const struct line* line,
                                            size_t column);

size_t displayed_string_length(const wchar_t* str, size_t len, size_t tabsize);
struct display_char* expand_tabs(const wchar_t* str, size_t len, uint8_t* colors,
                                 size_t colors_len, size_t* ret_len_ptr,
                                 size_t tabsize);
void render_editor(struct editor* editor, struct terminal_state* state);

#endif

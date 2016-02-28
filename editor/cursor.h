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

    cursor.h
    Editor cursor.

*******************************************************************************/

#ifndef EDITOR_CURSOR_H
#define EDITOR_CURSOR_H

#include <stddef.h>

struct editor;

size_t editor_select_column_set(struct editor* editor, size_t x);
size_t editor_select_row_set(struct editor* editor, size_t y);
void editor_select_set(struct editor* editor, size_t y, size_t x);
size_t editor_select_column_dec(struct editor* editor);
size_t editor_select_column_inc(struct editor* editor);
size_t editor_select_row_dec(struct editor* editor);
size_t editor_select_row_inc(struct editor* editor);

size_t editor_cursor_column_set(struct editor* editor, size_t x);
size_t editor_cursor_row_set(struct editor* editor, size_t y);
void editor_cursor_set(struct editor* editor, size_t y, size_t x);
size_t editor_cursor_column_dec(struct editor* editor);
size_t editor_cursor_column_inc(struct editor* editor);
size_t editor_cursor_row_dec(struct editor* editor);
size_t editor_cursor_row_inc(struct editor* editor);

#endif

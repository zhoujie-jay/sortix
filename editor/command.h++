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

    command.h++
    Editor commands.

*******************************************************************************/

#ifndef EDITOR_COMMAND_HXX
#define EDITOR_COMMAND_HXX

#include <stddef.h>

struct editor;

void editor_type_newline(struct editor* editor);
void editor_type_combine_with_last(struct editor* editor);
void editor_type_backspace(struct editor* editor);
void editor_type_combine_with_next(struct editor* editor);
void editor_type_delete(struct editor* editor);
void editor_type_delete_selection(struct editor* editor);
void editor_type_left(struct editor* editor);
void editor_type_select_left(struct editor* editor);
void editor_type_right(struct editor* editor);
void editor_type_select_right(struct editor* editor);
void editor_type_up(struct editor* editor);
void editor_type_select_up(struct editor* editor);
void editor_type_down(struct editor* editor);
void editor_type_select_down(struct editor* editor);
void editor_skip_leading(struct editor* editor);
void editor_select_skip_leading(struct editor* editor);
void editor_type_home(struct editor* editor);
void editor_type_select_home(struct editor* editor);
void editor_skip_ending(struct editor* editor);
void editor_select_skip_ending(struct editor* editor);
void editor_type_end(struct editor* editor);
void editor_type_select_end(struct editor* editor);
void editor_type_page_up(struct editor* editor);
void editor_type_select_page_up(struct editor* editor);
void editor_type_page_down(struct editor* editor);
void editor_type_select_page_down(struct editor* editor);
void editor_type_edit(struct editor* editor);
void editor_type_goto_line(struct editor* editor);
void editor_type_save(struct editor* editor);
void editor_type_save_as(struct editor* editor);
void editor_type_open(struct editor* editor);
void editor_type_open_as(struct editor* editor);
void editor_type_quit(struct editor* editor);
void editor_type_command(struct editor* editor);
void editor_type_raw_character(struct editor* editor, wchar_t c);
void editor_type_copy(struct editor* editor);
void editor_type_cut(struct editor* editor);
void editor_type_paste(struct editor* editor);
void editor_type_character(struct editor* editor, wchar_t c);

#endif

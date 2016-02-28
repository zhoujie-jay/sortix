/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2016.

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

    modal.h
    Modal commands.

*******************************************************************************/

#ifndef EDITOR_MODAL_H
#define EDITOR_MODAL_H

struct editor;

bool is_truth_string(const char* truth);
bool is_truth_true(const char* truth);

void editor_modal_left(struct editor* editor);
void editor_modal_right(struct editor* editor);
void editor_modal_home(struct editor* editor);
void editor_modal_end(struct editor* editor);
void editor_modal_backspace(struct editor* editor);
void editor_modal_delete(struct editor* editor);

void editor_modal_load(struct editor* editor, const char* path);
void editor_modal_save(struct editor* editor, const char* path);
void editor_modal_save_load(struct editor* editor, const char* path);
void editor_modal_save_quit(struct editor* editor, const char* path);
void editor_modal_ask_load(struct editor* editor, const char* answer);
void editor_modal_ask_quit(struct editor* editor, const char* answer);
void editor_modal_goto_line(struct editor* editor, const char* linestr);
void editor_modal_margin(struct editor* editor, const char* marginstr);
void editor_modal_popen(struct editor* editor, const char* cmd);
void editor_modal_tabsize(struct editor* editor, const char* tabsizestr);
void editor_modal_language(struct editor* editor, const char* language);
void editor_modal_line_numbering(struct editor* editor, const char* truth);

bool is_modal_command(const char* cmd, const char* candidate, const char** rest);
void editor_modal_command(struct editor* editor, const char* cmd);
void editor_modal_command_config(struct editor* editor, const char* cmd);
void editor_modal_character(struct editor* editor, wchar_t c);

#endif

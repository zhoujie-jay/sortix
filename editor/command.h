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
 * command.h
 * Editor commands.
 */

#ifndef EDITOR_COMMAND_H
#define EDITOR_COMMAND_H

#include <stddef.h>

struct editor;

void editor_type_newline(struct editor* editor);
void editor_type_combine_with_last(struct editor* editor);
void editor_type_backspace(struct editor* editor);
void editor_type_combine_with_next(struct editor* editor);
void editor_type_delete(struct editor* editor);
void editor_type_delete_selection(struct editor* editor);
void editor_type_exit_select_left(struct editor* editor);
void editor_type_exit_select_right(struct editor* editor);
void editor_type_left(struct editor* editor);
void editor_type_select_left(struct editor* editor);
void editor_type_control_left(struct editor* editor);
void editor_type_control_select_left(struct editor* editor);
void editor_type_right(struct editor* editor);
void editor_type_select_right(struct editor* editor);
void editor_type_control_right(struct editor* editor);
void editor_type_control_select_right(struct editor* editor);
void editor_type_up(struct editor* editor);
void editor_type_select_up(struct editor* editor);
void editor_type_control_up(struct editor* editor);
void editor_type_control_select_up(struct editor* editor);
void editor_type_down(struct editor* editor);
void editor_type_select_down(struct editor* editor);
void editor_type_control_down(struct editor* editor);
void editor_type_control_select_down(struct editor* editor);
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

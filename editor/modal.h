/*
 * Copyright (c) 2013, 2014, 2016 Jonas 'Sortie' Termansen.
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
 * modal.h
 * Modal commands.
 */

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

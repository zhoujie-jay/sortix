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
 * display.h
 * Display handling.
 */

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

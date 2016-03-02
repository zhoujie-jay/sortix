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
 * terminal.h
 * Terminal handling.
 */

#ifndef EDITOR_TERMINAL_H
#define EDITOR_TERMINAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct terminal_datum
{
	wchar_t character;
	uint8_t vgacolor;
};

__attribute__((unused))
static inline
struct terminal_datum make_terminal_datum(wchar_t c, uint8_t cl)
{
	struct terminal_datum result = { c, cl };
	return result;
}

struct terminal_state
{
	int width;
	int height;
	int cursor_x;
	int cursor_y;
	uint8_t color;
	struct terminal_datum* data;
};

__attribute__((unused))
static inline
bool is_row_column_lt(size_t ra, size_t ca, size_t rb, size_t cb)
{
	return ra < rb || (ra == rb && ca < cb);
}

__attribute__((unused))
static inline
bool is_row_column_le(size_t ra, size_t ca, size_t rb, size_t cb)
{
	return (ra == rb && ca == cb) || is_row_column_lt(ra, ca, rb, cb);
}

__attribute__((unused))
static inline
void row_column_smallest(size_t ra, size_t ca, size_t rb, size_t cb,
                         size_t* row, size_t* column)
{
	if ( is_row_column_lt(ra, ca, rb, cb) )
		*row = ra, *column = ca;
	else
		*row = rb, *column = cb;
}

__attribute__((unused))
static inline
void row_column_biggest(size_t ra, size_t ca, size_t rb, size_t cb,
                         size_t* row, size_t* column)
{
	if ( is_row_column_lt(ra, ca, rb, cb) )
		*row = rb, *column = cb;
	else
		*row = ra, *column = ca;
}

void update_terminal_color(FILE* fp, uint8_t desired_color,
                           struct terminal_state* current);
void update_terminal_cursor(FILE* fp, int x, int y,
                            struct terminal_state* current);
void update_terminal_entry(FILE* fp, struct terminal_datum entry, int x, int y,
                           struct terminal_state* current);
void update_terminal(FILE* fp,
                     struct terminal_state* desired,
                     struct terminal_state* current);
void make_terminal_state(FILE* fp, struct terminal_state* state);
void free_terminal_state(struct terminal_state* state);
void reset_terminal_state(FILE* fp, struct terminal_state* state);


#endif

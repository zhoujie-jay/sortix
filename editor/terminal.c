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
 * terminal.c
 * Terminal handling.
 */

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <termios.h>

#include "terminal.h"

void update_terminal_color(FILE* fp, uint8_t desired_color,
                           struct terminal_state* current)
{
	uint8_t desired_fg = (desired_color >> 0) % 16;
	uint8_t desired_bg = (desired_color >> 4) % 16;
	uint8_t current_fg = (current->color >> 0) % 16;
	uint8_t current_bg = (current->color >> 4) % 16;
	if ( desired_fg != current_fg )
		fprintf(fp, "\e[%im", desired_fg + (desired_fg < 8 ? 30 : 90-8) );
	if ( desired_bg != current_bg )
		fprintf(fp, "\e[%im", desired_bg + (desired_bg < 8 ? 40 : 100-8) );
	current->color = desired_color;
}

void update_terminal_cursor(FILE* fp, int x, int y,
                            struct terminal_state* current)
{
	if ( current->cursor_x == x && current->cursor_y == y )
		return;
	fprintf(fp, "\e[%i;%iH", y + 1, x + 1);
	current->cursor_x = x;
	current->cursor_y = y;
}

void update_terminal_entry(FILE* fp, struct terminal_datum entry, int x, int y,
                           struct terminal_state* current)
{
	assert(entry.character != L'\0');
	size_t index = y * current->width + x;
	struct terminal_datum current_entry = current->data[index];
	if ( entry.character == current_entry.character &&
	     entry.vgacolor == current_entry.vgacolor )
		return;
	update_terminal_cursor(fp, x, y, current);
	update_terminal_color(fp, entry.vgacolor, current);
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	char mb[MB_CUR_MAX];
	size_t count = wcrtomb(mb, entry.character, &ps);
	if ( count == (size_t) -1 )
		fputs("�", fp);
	else if ( entry.character == L'\b' ||
	          entry.character == L'\e' ||
	          entry.character == L'\n' ||
	          entry.character == L'\r' ||
	          entry.character == L'\t' )
		fputs("�", fp);
	else for ( size_t i = 0; i < count; i++ )
		fputc(mb[i], fp);
	current->data[index] = entry;
	if ( ++current->cursor_x == current->width )
	{
		current->cursor_x = 0;
		current->cursor_y++;
	}
}

void update_terminal(FILE* fp,
                     struct terminal_state* desired,
                     struct terminal_state* current)
{
	// TODO: If terminal size has changed!
	for ( int y = 0; y < current->height; y++ )
	{
		for ( int x = 0; x < current->width; x++ )
		{
			size_t index = y * desired->width + x;
			struct terminal_datum desired_entry = desired->data[index];
			update_terminal_entry(fp, desired_entry, x, y, current);
		}
	}
	update_terminal_cursor(fp, desired->cursor_x, desired->cursor_y, current);
	update_terminal_color(fp, desired->color, current);
}

void make_terminal_state(FILE* fp, struct terminal_state* state)
{
	memset(state, 0, sizeof(*state));

	struct winsize terminal_size;
	tcgetwinsize(fileno(fp), &terminal_size);
	state->width = (int) terminal_size.ws_col;
	state->height = (int) terminal_size.ws_row;
	size_t data_length = state->width * state->height;
	size_t data_size = sizeof(struct terminal_datum) * data_length;
	state->data = (struct terminal_datum*) malloc(data_size);
	for ( size_t i = 0; i < data_length; i++ )
		state->data[i].character = L' ',
		state->data[i].vgacolor = 0;
}

void free_terminal_state(struct terminal_state* state)
{
	free(state->data);
}

void reset_terminal_state(FILE* fp, struct terminal_state* state)
{
	fprintf(fp, "\e[H");
	fprintf(fp, "\e[m");
	fprintf(fp, "\e[2J");
	state->cursor_x = 0;
	state->cursor_y = 0;

	update_terminal_color(fp, 0x07, state);
	for ( int y = 0; y < state->height; y++ )
		for ( int x = 0; x < state->width; x++ )
			update_terminal_entry(fp, make_terminal_datum(L' ', 0x07), x, y, state);

	update_terminal_cursor(fp, 0, 0, state);
}

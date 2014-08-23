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

    terminal.h++
    Terminal handling.

*******************************************************************************/

#ifndef EDITOR_TERMINAL_HXX
#define EDITOR_TERMINAL_HXX

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

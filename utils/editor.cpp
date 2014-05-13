/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    editor.cpp
    A simple text editor.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct terminal_state
{
	int width;
	int height;
	int cursor_x;
	int cursor_y;
	uint8_t color;
	uint16_t* data;
};

char* strdup_safe(const char* str)
{
	return strdup(str ? str : "");
}

size_t displayed_string_length(const char* str, size_t len, size_t tabsize)
{
	size_t ret_len = 0;
	for ( size_t i = 0; i < len; i++ )
		if ( str[i] == '\t' )
			do ret_len++;
			while ( ret_len % tabsize );
		else
			ret_len++;
	return ret_len;
}

struct display_char
{
	char character;
	uint8_t color;
};

struct display_char* expand_tabs(const char* str, size_t len, uint8_t* colors,
                                 size_t colors_len, size_t* ret_len_ptr,
                                 size_t tabsize)
{
	size_t ret_len = displayed_string_length(str, len, tabsize);
	struct display_char* ret = new struct display_char[ret_len+1];
	for ( size_t i = 0, j = 0; i < len; i++ )
	{
		uint8_t color = i < colors_len ? colors[i] : 7;
		if ( str[i] == '\t' )
			do ret[j++] = { ' ', color};
			while ( j % tabsize );
		else
			ret[j++] = { str[i], color };
	}
	ret[ret_len] = { '\0', 0 };
	if ( ret_len_ptr )
		*ret_len_ptr = ret_len;
	return ret;
}

bool is_row_column_lt(size_t ra, size_t ca, size_t rb, size_t cb)
{
	return ra < rb || (ra == rb && ca < cb);
}

bool is_row_column_le(size_t ra, size_t ca, size_t rb, size_t cb)
{
	return (ra == rb && ca == cb) || is_row_column_lt(ra, ca, rb, cb);
}

void row_column_smallest(size_t ra, size_t ca, size_t rb, size_t cb,
                         size_t* row, size_t* column)
{
	if ( is_row_column_lt(ra, ca, rb, cb) )
		*row = ra, *column = ca;
	else
		*row = rb, *column = cb;
}

void row_column_biggest(size_t ra, size_t ca, size_t rb, size_t cb,
                         size_t* row, size_t* column)
{
	if ( is_row_column_lt(ra, ca, rb, cb) )
		*row = rb, *column = cb;
	else
		*row = ra, *column = ca;
}

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

void update_terminal_entry(FILE* fp, uint16_t entry, int x, int y,
                           struct terminal_state* current)
{
	assert(entry & 0xFF);
	size_t index = y * current->width + x;
	uint16_t current_entry = current->data[index];
	if ( entry == current_entry )
		return;
	update_terminal_cursor(fp, x, y, current);
	uint8_t color = entry >> 8;
	update_terminal_color(fp, color, current);
	fputc((char) (entry & 0xFF), fp);
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
			uint16_t desired_entry = desired->data[index];
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
	state->data = (uint16_t*) malloc(sizeof(uint16_t) * data_length);
	for ( size_t i = 0; i < data_length; i++ )
		state->data[i] = 0x0000 | ' ';
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
			update_terminal_entry(fp, 0x0700 | ' ', x, y, state);

	update_terminal_cursor(fp, 0, 0, state);
}

struct line
{
	char* data;
	size_t used;
	size_t length;
};

struct color_line
{
	uint8_t* data;
	size_t length;
};

enum editor_mode
{
	MODE_QUIT,
	MODE_EDIT,
	MODE_LOAD,
	MODE_SAVE,
	MODE_ASK_QUIT,
	MODE_GOTO_LINE,
	MODE_COMMAND,
};

struct editor
{
	char* current_file_name;
	struct line* lines;
	size_t lines_used;
	size_t lines_length;
	struct color_line* color_lines;
	size_t color_lines_used;
	size_t color_lines_length;
	size_t cursor_column;
	size_t cursor_row;
	size_t select_column;
	size_t select_row;
	size_t viewport_width;
	size_t viewport_height;
	size_t page_x_offset;
	size_t page_y_offset;
	char* modal;
	size_t modal_used;
	size_t modal_length;
	size_t modal_cursor;
	char* clipboard;
	size_t tabsize;
	size_t margin;
	enum editor_mode mode;
	bool control;
	bool shift;
	bool lshift;
	bool rshift;
	bool dirty;
	bool modal_error;
	bool highlight_source;
};

void initialize_editor(struct editor* editor)
{
	editor->current_file_name = NULL;
	editor->lines = NULL;
	editor->lines_used = 0;
	editor->lines_length = 0;
	editor->cursor_column = 0;
	editor->cursor_row = 0;
	editor->select_column = 0;
	editor->select_row = 0;
	editor->page_x_offset = 0;
	editor->page_y_offset = 0;
	editor->modal = NULL;
	editor->modal_used = 0;
	editor->modal_length = 0;
	editor->modal_cursor = 0;
	editor->clipboard = NULL;
	editor->tabsize = 8;
	editor->margin = SIZE_MAX;
	editor->mode = MODE_EDIT;
	editor->control = false;
	editor->shift = false;
	editor->lshift = false;
	editor->rshift = false;
	editor->dirty = false;
	editor->modal_error = false;
	editor->highlight_source = false;

	editor->lines_used = 1;
	editor->lines_length = 1;
	editor->lines = new struct line[editor->lines_length];
	editor->lines[0].data = NULL;
	editor->lines[0].used = 0;
	editor->lines[0].length = 0;

	editor->color_lines_used = 0;
	editor->color_lines_length = 0;
	editor->color_lines = NULL;
}

bool editor_has_selection(struct editor* editor)
{
	return !(editor->cursor_row == editor->select_row &&
	         editor->cursor_column == editor->select_column);
}

void render_editor(struct editor* editor, struct terminal_state* state)
{
	if ( state->height < 1 )
		return;

	// Create the header title bar.
	for ( int x = 0; x < state->width; x++ )
		state->data[0 * state->width + x] = 0x7000 | ' ';

	// Render the name of the program.
	const char* header_start = editor->dirty ? "  editor          *"
	                                         : "  editor           ";
	size_t header_start_len = strlen(header_start);

	for ( size_t i = 0; i < header_start_len; i++ )
		if ( i < (size_t) state->width)
			state->data[i] |= (unsigned char) header_start[i];

	// Render the name of the currently open file.
	const char* file_name = editor->current_file_name;
	if ( !file_name )
		file_name = "New File";
	size_t file_name_len = strlen(file_name);

	for ( size_t i = 0; i < file_name_len; i++ )
		if ( header_start_len+i < (size_t) state->width)
			state->data[header_start_len+i] |= (unsigned char) file_name[i];

	// Calculate the dimensions of the viewport.
	size_t viewport_top = 1;
	editor->viewport_width = (size_t) state->width;
	editor->viewport_height = (size_t) state->height - viewport_top;
	if ( !editor->viewport_height )
		return;

	// Decide which page of the file to render and the cursor position on it.
	struct line* current_line = &editor->lines[editor->cursor_row];
	size_t cursor_x = displayed_string_length(current_line->data,
	                                          editor->cursor_column,
	                                          editor->tabsize);
	size_t cursor_y = editor->cursor_row;
	struct line* select_line = &editor->lines[editor->select_row];
	size_t select_x = displayed_string_length(select_line->data,
	                                          editor->select_column,
	                                          editor->tabsize);
	size_t select_y = editor->select_row;

	size_t page_x_offset = editor->page_x_offset;
	size_t page_y_offset = editor->page_y_offset;

	bool has_selection = !(editor->cursor_row == editor->select_row &&
	                       editor->cursor_column == editor->select_column);
	size_t viewport_select_x = select_x - page_x_offset;
	size_t viewport_select_y = select_y - page_y_offset;

	// Render this page of text.
	for ( size_t y = 0; y < editor->viewport_height; y++ )
	{
		size_t line_index = page_y_offset + y;
		uint16_t* data_line = state->data + (viewport_top + y) * state->width;
		struct line* line = line_index < editor->lines_used ?
		                    &editor->lines[line_index] : NULL;
		struct color_line* color_line = line_index < editor->color_lines_used ?
		                                &editor->color_lines[line_index] : NULL;
		size_t expanded_len;
		struct display_char* expanded
			= expand_tabs(line ? line->data : "",
			              line ? line->used : 0,
			              color_line ? color_line->data : NULL,
			              color_line ? color_line->length : 0,
			              &expanded_len,
			              editor->tabsize);
		const struct display_char* chars = expanded;
		size_t chars_length = expanded_len;
		if ( chars_length < page_x_offset )
			chars = NULL, chars_length = 0;
		else
			chars += page_x_offset, chars_length -= page_x_offset;
		for ( size_t x = 0; x < editor->viewport_width; x++ )
		{
			size_t column_index = page_x_offset + x;
			bool selected = (is_row_column_lt(cursor_y, cursor_x, select_y, select_x) &&
			                 is_row_column_le(cursor_y, cursor_x, line_index, column_index) &&
			                 is_row_column_lt(line_index, column_index, select_y, select_x)) ||
                            (is_row_column_lt(select_y, select_x, cursor_y, cursor_x) &&
			                 is_row_column_le(select_y, select_x, line_index, column_index) &&
			                 is_row_column_lt(line_index, column_index, cursor_y, cursor_x));
			bool at_margin = column_index == editor->margin;
			bool is_blank = chars_length <= x;
			char c = is_blank ? ' ' : chars[x].character;
			uint8_t color = (is_blank ? 7 : chars[x].color);
			data_line[x] = selected && is_blank && at_margin ? 0x4100 | '|' :
                           selected ? 0x4700 | (unsigned char) c :
			               is_blank && at_margin ? 0x0100 | '|' :
			               color << 8 | (unsigned char) c;
		}
		delete[] expanded;
	}

	// Set the rest of the terminal state.
	state->cursor_x = has_selection ?
	                  editor->viewport_width : viewport_select_x;
	state->cursor_y = has_selection ?
	                  editor->viewport_height : viewport_select_y + viewport_top;
	state->color = 0x07;

	if ( editor->mode == MODE_EDIT )
		return;

	const char* msg = "";
	if ( editor->mode == MODE_SAVE )
		msg = "File Name to Write: ";
	if ( editor->mode == MODE_LOAD )
		msg = "File Name to Read: ";;
	if ( editor->mode == MODE_ASK_QUIT )
		msg = "Exit without saving changes? (Y/N): ";
	if ( editor->mode == MODE_GOTO_LINE )
		msg = "Go to line: ";
	if ( editor->mode == MODE_COMMAND )
		msg = "Enter miscellaneous command: ";
	size_t msg_len = strlen(msg);

	uint16_t* data_line = state->data + (state->height - 1) * state->width;
	for ( size_t i = 0; i < msg_len; i++ )
		if ( i < (size_t) state->width)
			data_line[i] = 0x7000 | (unsigned char) msg[i];

	if ( (size_t) state->width <= msg_len )
		return;

	size_t modal_viewport_width = state->width - msg_len;
	size_t modal_viewport_cursor = editor->modal_cursor % modal_viewport_width;
	size_t modal_viewport_page = editor->modal_cursor / modal_viewport_width;
	size_t modal_viewport_offset = modal_viewport_page * modal_viewport_width;

	uint16_t* modal_viewport_data = data_line + msg_len;

	const char* modal_chars = editor->modal;
	size_t modal_chars_length = editor->modal_used;
	if ( modal_chars_length < modal_viewport_offset )
		modal_chars = NULL,
		modal_chars_length = 0;
	else
		modal_chars += modal_viewport_offset,
		modal_chars_length -= modal_viewport_offset;

	for ( size_t x = 0; x < modal_viewport_width; x++ )
	{
		char c = x < modal_chars_length ? modal_chars[x] : ' ';
		uint16_t color = editor->modal_error ? 0x1700 : 0x7000;
		uint16_t tab_color = editor->modal_error ? 0x1200 : 0x7100;
		if ( c == '\t' )
			modal_viewport_data[x] = tab_color | (unsigned char) '>';
		else
			modal_viewport_data[x] = color | (unsigned char) c;
	}

	state->cursor_x = msg_len + modal_viewport_cursor;
	state->cursor_y = state->height - 1;
	state->color = 0x70;
}

size_t recognize_constant(const char* string, size_t string_length)
{
	bool hex = false;
	size_t result = 0;
	if ( result < string_length && string[result] == '0' )
	{
		result++;
		if ( result < string_length && (string[result] == 'x' ||
		                                string[result] == 'X') )
		{
			result++;
			hex = true;
		}
	}
	bool floating = false;
	bool exponent = false;
	while ( result < string_length )
	{
		if ( ('0' <= string[result] && string[result] <= '9') ||
		     (hex && 'a' <= string[result] && string[result] <= 'f') ||
		     (hex && 'A' <= string[result] && string[result] <= 'F') )
		{
			result++;
			continue;
		}
		if ( string[result] == '.' )
		{
			if ( hex || floating )
				return 0;
			floating = true;
			result++;
			continue;
		}
		if ( !hex && (string[result] == 'e' || string[result] == 'E') )
		{
			if ( !result )
				return 0;
			if ( exponent )
				return 0;
			floating = true;
			result++;
			continue;
		}
		break;
	}
	if ( result == (hex ? 2 : 0) )
		return 0;
	if ( floating )
	{
		if ( result < string_length && (string[result] == 'l' ||
			                            string[result] == 'L') )
			result++;
		else if ( result < string_length && (string[result] == 'f' ||
			                                 string[result] == 'F') )
			result++;
	}
	else
	{
		if ( result < string_length && (string[result] == 'u' ||
			                            string[result] == 'U') )
			result++;
		if ( result < string_length && (string[result] == 'l' ||
			                            string[result] == 'L') )
			result++;
		if ( result < string_length && (string[result] == 'l' ||
			                            string[result] == 'L') )
			result++;
	}
	return result;
}

void editor_colorize(struct editor* editor)
{
	if ( editor->color_lines_length != editor->lines_used ||
	     !editor->highlight_source )
	{
		for ( size_t i = 0; i < editor->color_lines_used; i++ )
			delete[] editor->color_lines[i].data;
		delete[] editor->color_lines;
		editor->color_lines_used = 0;
		editor->color_lines_length = 0;
		editor->color_lines = NULL;
	}

	if ( !editor->highlight_source )
		return;

	if ( !editor->color_lines )
	{
		if ( !(editor->color_lines = new struct color_line[editor->lines_used]) )
			return;
		editor->color_lines_used = editor->lines_used;
		editor->color_lines_length = editor->lines_used;
		for ( size_t i = 0; i < editor->lines_used; i++ )
			editor->color_lines[i].data = NULL,
			editor->color_lines[i].length = 0;
	}

	for ( size_t i = 0; i < editor->lines_used; i++ )
	{
		if ( editor->color_lines[i].length == editor->lines[i].used )
			continue;

		if ( !(editor->color_lines[i].data = new uint8_t[editor->lines[i].used]) )
		{
			for ( size_t n = 0; n < i; i++ )
				delete[] editor->color_lines[n].data;
			delete[] editor->color_lines;
			editor->color_lines_used = 0;
			editor->color_lines_length = 0;
			editor->color_lines = NULL;
			return;
		}

		editor->color_lines[i].length = editor->lines[i].used;
	}

	enum
	{
		STATE_INIT,
		STATE_LINE_COMMENT,
		STATE_MULTI_LINE_COMMENT,
		STATE_PREPROCESSOR,
		STATE_SINGLE_QUOTE,
		STATE_DOUBLE_QUOTE,
		STATE_NUMBER,
		STATE_KEYWORD,
		STATE_TYPE,
	} state = STATE_INIT, prev_state = STATE_INIT;

	bool escaped = false;
	size_t fixed_state = 0;
	size_t multi_expiration = 0;
	for ( size_t y = 0; y < editor->lines_used; y++ )
	{
		struct line* line = &editor->lines[y];
		for ( size_t x = 0; x < line->used; x++ )
		{
			char pc = x ? line->data[x-1] : '\0';
			char c = line->data[x];
			char nc = x+1 < line->used ? line->data[x+1] : '\0';
			uint8_t color = 7;

			// The character makes you leave this state.

			if ( !fixed_state && (state == STATE_KEYWORD ||
			                      state == STATE_TYPE ||
			                      state == STATE_NUMBER ) )
				state = STATE_INIT;

			// The character makes you enter a new state.

			if ( !fixed_state && state == STATE_INIT && c == '#' )
				state = STATE_PREPROCESSOR;

			// TODO: Detect NULL as a value.

			if ( !fixed_state && state == STATE_INIT &&
			     !(x && (isalnum(pc) || pc == '_')) )
			{
				size_t number_length = recognize_constant(line->data + x,
				                                          line->used - x);
				if ( number_length )
				{
					state = STATE_NUMBER;
					fixed_state = number_length;
				}
			}

			if ( !fixed_state && state == STATE_INIT && c == '\'' )
				state = STATE_SINGLE_QUOTE, fixed_state = 1, escaped = false;

			if ( !fixed_state && state == STATE_INIT && c == '"' )
				state = STATE_DOUBLE_QUOTE, fixed_state = 1, escaped = false;

			if ( !fixed_state && (state == STATE_INIT ||
			                      state == STATE_PREPROCESSOR) )
			{
				if ( c == '/' && nc == '/' )
					state = STATE_LINE_COMMENT, fixed_state = 2;
				else if ( c == '/' && nc == '*' )
				{
					prev_state = state;
					multi_expiration = 0;
					state = STATE_MULTI_LINE_COMMENT;
					fixed_state = 2;
				}
			}

			if ( !fixed_state && state == STATE_INIT )
			{
				const char* keywords[] =
				{
					"alignas",
					"alignof",
					"and",
					"and_eq",
					"asm",
					"bitand",
					"bitor",
					"break",
					"case",
					"catch",
					"class",
					"compl",
					"const_cast",
					"constexpr",
					"continue",
					"decltype",
					"default",
					"delete",
					"do",
					"dynamic_cast",
					"else",
					"enum",
					"false",
					"final",
					"for",
					"friend",
					"goto",
					"if",
					"namespace",
					"new",
					"not",
					"not_eq",
					"nullptr",
					"operator",
					"or",
					"or_eq",
					"override",
					"private",
					"protected",
					"public",
					"reinterpret_cast",
					"return",
					"sizeof",
					"static_assert",
					"static_cast",
					"struct",
					"switch",
					"template",
					"this",
					"thread_local",
					"throw",
					"true",
					"try",
					"typedef",
					"typeid",
					"typename",
					"union",
					"using",
					"virtual",
					"while",
					"xor",
					"xor_eq",
				};

				bool cannot_be_keyword = x && (isalnum(pc) || pc == '_');
				for ( size_t i = 0;
				      !cannot_be_keyword && i < sizeof(keywords) / sizeof(keywords[0]);
				      i++ )
				{
					const char* keyword = keywords[i];
					if ( c != keyword[0] )
						continue;
					size_t keyword_length = strlen(keyword);
					if ( (x - line->used) < keyword_length )
						continue;
					if ( strncmp(line->data + x, keyword, keyword_length) != 0 )
						continue;
					if ( (x - line->used) != keyword_length &&
					     (isalnum(line->data[x+keyword_length]) ||
					      line->data[x+keyword_length] == '_') )
						continue;
					state = STATE_KEYWORD;
					fixed_state = keyword_length;
				}
			}

			if ( !fixed_state && state == STATE_INIT )
			{
				const char* types[] =
				{
					"auto",
					"blkcnt_t",
					"blksize_t",
					"bool",
					"char",
					"char16_t",
					"char32_t",
					"clockid_t",
					"clock_t",
					"const",
					"dev_t",
					"double",
					"explicit",
					"extern",
					"FILE",
					"float",
					"fpos_t",
					"fsblkcnt_t",
					"fsfilcnt_t",
					"gid_t",
					"id_t",
					"inline",
					"ino_t",
					"int",
					"int16_t",
					"int32_t",
					"int64_t",
					"int8_t",
					"intmax_t",
					"intptr_t",
					"locale_t",
					"long",
					"mode_t",
					"mutable",
					"nlink_t",
					"noexcept",
					"off_t",
					"pid_t",
					"ptrdiff_t",
					"register",
					"restrict",
					"short",
					"signed",
					"size_t",
					"ssize_t",
					"static",
					"suseconds_t",
					"thread_local",
					"timer_t",
					"time_t",
					"trace_t",
					"uid_t",
					"uint16_t",
					"uint32_t",
					"uint64_t",
					"uint8_t",
					"uintmax_t",
					"uintptr_t",
					"unsigned",
					"useconds_t",
					"va_list",
					"void",
					"volatile",
					"wchar_t",
				};

				bool cannot_be_type = x && (isalnum(pc) || pc == '_');
				for ( size_t i = 0;
				      !cannot_be_type && i < sizeof(types) / sizeof(types[0]);
				      i++ )
				{
					const char* type = types[i];
					if ( c != type[0] )
						continue;
					size_t type_length = strlen(type);
					if ( (x - line->used) < type_length )
						continue;
					if ( strncmp(line->data + x, type, type_length) != 0 )
						continue;
					if ( (x - line->used) != type_length &&
					     (isalnum(line->data[x+type_length]) ||
					      line->data[x+type_length] == '_') )
						continue;
					state = STATE_TYPE;
					fixed_state = type_length;
				}
			}

			// The current state uses a non-default color.

			if ( state == STATE_SINGLE_QUOTE ||
			     state == STATE_DOUBLE_QUOTE ||
			     state == STATE_NUMBER )
				color = 5;

			if ( state == STATE_PREPROCESSOR )
				color = 3;

			if ( state == STATE_LINE_COMMENT ||
			     state == STATE_MULTI_LINE_COMMENT )
				color = 6;

			if ( state == STATE_KEYWORD )
				color = 1;

			if ( state == STATE_TYPE )
				color = 2;

			// The character is the last character in this state.

			if ( !fixed_state )
			{
				if ( state == STATE_SINGLE_QUOTE && !escaped && c == '\'' )
					state = STATE_INIT, fixed_state = 1;
				if ( state == STATE_DOUBLE_QUOTE && !escaped && c == '"' )
					state = STATE_INIT, fixed_state = 1;
			}

			if ( (state == STATE_SINGLE_QUOTE || state == STATE_DOUBLE_QUOTE) )
			{
				if ( !escaped && c == '\\' )
					escaped = true;
				else if ( escaped )
					escaped = false;
			}

			if ( !fixed_state && state == STATE_MULTI_LINE_COMMENT )
			{
				if ( multi_expiration ==  1 )
					state = prev_state, multi_expiration = 0;
				else if ( c == '*' && nc == '/' )
					multi_expiration = 1;
			}

			if ( state == STATE_PREPROCESSOR )
				escaped = c == '\\' && !nc;

			editor->color_lines[y].data[x] = color;

			if ( fixed_state )
				fixed_state--;
		}

		if ( state == STATE_LINE_COMMENT ||
		     state == STATE_PREPROCESSOR ||
		     state == STATE_SINGLE_QUOTE ||
		     state == STATE_DOUBLE_QUOTE )
		{
			if ( state == STATE_PREPROCESSOR && escaped )
				escaped = false;
			else
				state = STATE_INIT;
		}
	}
}

size_t editor_select_column_set(struct editor* editor, size_t x)
{
	if ( editor->viewport_width )
	{
		struct line* line = &editor->lines[editor->select_row];
		size_t rx = displayed_string_length(line->data, x, editor->tabsize);
		if ( rx < editor->page_x_offset )
			editor->page_x_offset = rx;
		if ( editor->page_x_offset + editor->viewport_width <= rx )
			editor->page_x_offset = rx + 1- editor->viewport_width;
	}
	return editor->select_column = x;
}

size_t editor_select_row_set(struct editor* editor, size_t y)
{
	if ( editor->viewport_height )
	{
		if ( y < editor->page_y_offset )
			editor->page_y_offset = y;
		if ( editor->page_y_offset + editor->viewport_height <= y )
			editor->page_y_offset = y + 1- editor->viewport_height;
	}
	return editor->select_row = y;
}

void editor_select_set(struct editor* editor, size_t y, size_t x)
{
	editor_select_column_set(editor, x);
	editor_select_row_set(editor, y);
}

size_t editor_select_column_dec(struct editor* editor)
{
	assert(editor->select_column);
	return editor_select_column_set(editor, editor->select_column-1);
}

size_t editor_select_column_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_select_column_set(editor, editor->select_column+1);
}

size_t editor_select_row_dec(struct editor* editor)
{
	assert(editor->select_row);
	return editor_select_row_set(editor, editor->select_row-1);
}

size_t editor_select_row_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_select_row_set(editor, editor->select_row+1);
}

size_t editor_cursor_column_set(struct editor* editor, size_t x)
{
	editor_select_column_set(editor, x);
	editor_select_row_set(editor, editor->cursor_row);
	return editor->cursor_column = x;
}

size_t editor_cursor_row_set(struct editor* editor, size_t y)
{
	editor_select_column_set(editor, editor->cursor_column);
	editor_select_row_set(editor, y);
	return editor->cursor_row = y;
}

void editor_cursor_set(struct editor* editor, size_t y, size_t x)
{
	editor_cursor_column_set(editor, x);
	editor_cursor_row_set(editor, y);
}

size_t editor_cursor_column_dec(struct editor* editor)
{
	assert(editor->cursor_column);
	return editor_cursor_column_set(editor, editor->cursor_column-1);
}

size_t editor_cursor_column_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_cursor_column_set(editor, editor->cursor_column+1);
}

size_t editor_cursor_row_dec(struct editor* editor)
{
	assert(editor->cursor_row);
	return editor_cursor_row_set(editor, editor->cursor_row-1);
}

size_t editor_cursor_row_inc(struct editor* editor)
{
	// TODO: Assert line doesn't overflow!
	return editor_cursor_row_set(editor, editor->cursor_row+1);
}

void editor_type_newline(struct editor* editor)
{
	editor->dirty = true;

	if ( editor->lines_used == editor->lines_length )
	{
		size_t new_length = editor->lines_length ? 2 * editor->lines_length : 8;
		struct line* new_lines = new struct line[new_length];
		for ( size_t i = 0; i < editor->lines_used; i++ )
			new_lines[i] = editor->lines[i];
		delete[] editor->lines;
		editor->lines = new_lines;
		editor->lines_length = new_length;
	}

	for ( size_t i = editor->lines_used-1; editor->cursor_row < i; i-- )
		editor->lines[i+1] = editor->lines[i];
	editor->lines_used++;

	struct line old_line = editor->lines[editor->cursor_row];

	size_t keep_length = editor->cursor_column;
	size_t move_length = old_line.used - keep_length;

	struct line* keep_line = &editor->lines[editor->cursor_row];
	struct line* move_line = &editor->lines[editor->cursor_row+1];

	keep_line->data = new char[keep_length];
	keep_line->used = keep_length;
	keep_line->length = keep_length;
	memcpy(keep_line->data, old_line.data + 0, keep_length);

	move_line->data = new char[move_length];
	move_line->used = move_length;
	move_line->length = move_length;
	memcpy(move_line->data, old_line.data + keep_length, move_length);

	editor_cursor_set(editor, editor->cursor_row+1, 0);

	delete[] old_line.data;
}

void editor_type_delete_selection(struct editor* editor);

void editor_type_combine_with_last(struct editor* editor)
{
	if ( !editor->cursor_row )
		return;

	editor->dirty = true;

	struct line* keep_line = &editor->lines[editor->cursor_row-1];
	struct line* gone_line = &editor->lines[editor->cursor_row];

	char* keep_line_data = keep_line->data;
	char* gone_line_data = gone_line->data;

	size_t new_length = keep_line->used + gone_line->used;
	char* new_data = new char[new_length];

	memcpy(new_data, keep_line_data, keep_line->used);
	memcpy(new_data + keep_line->used, gone_line_data, gone_line->used);

	editor_cursor_set(editor, editor->cursor_row-1, keep_line->used);

	keep_line->data = new_data;
	keep_line->used = new_length;
	keep_line->length = new_length;

	editor->lines_used--;
	for ( size_t i = editor->cursor_row + 1; i < editor->lines_used; i++ )
		editor->lines[i] = editor->lines[i+1];

	free(keep_line_data);
	free(gone_line_data);
}

void editor_type_backspace(struct editor* editor)
{
	if ( !(editor->select_row == editor->cursor_row &&
	       editor->select_column == editor->cursor_column) )
	{
		editor_type_delete_selection(editor);
		return;
	}

	struct line* current_line = &editor->lines[editor->cursor_row];

	if ( !editor->cursor_column )
	{
		editor_type_combine_with_last(editor);
		return;
	}

	editor->dirty = true;

	current_line->used--;
	for ( size_t i = editor_cursor_column_dec(editor); i < current_line->used; i++ )
		current_line->data[i] = current_line->data[i+1];
}

void editor_type_combine_with_next(struct editor* editor)
{
	if ( editor->cursor_row + 1 == editor->lines_used )
		return;

	editor->dirty = true;

	struct line* keep_line = &editor->lines[editor->cursor_row];
	struct line* gone_line = &editor->lines[editor->cursor_row+1];

	char* keep_line_data = keep_line->data;
	char* gone_line_data = gone_line->data;

	size_t new_length = keep_line->used + gone_line->used;
	char* new_data = new char[new_length];

	memcpy(new_data, keep_line_data, keep_line->used);
	memcpy(new_data + keep_line->used, gone_line_data, gone_line->used);

	editor_cursor_column_set(editor, keep_line->used);

	keep_line->data = new_data;
	keep_line->used = new_length;
	keep_line->length = new_length;

	editor->lines_used--;
	for ( size_t i = editor->cursor_row + 1; i < editor->lines_used; i++ )
		editor->lines[i] = editor->lines[i+1];

	free(keep_line_data);
	free(gone_line_data);
}

void editor_type_delete(struct editor* editor)
{
	if ( !(editor->select_row == editor->cursor_row &&
	       editor->select_column == editor->cursor_column) )
	{
		editor_type_delete_selection(editor);
		return;
	}

	struct line* current_line = &editor->lines[editor->cursor_row];

	if ( editor->cursor_column == current_line->used )
	{
		editor_type_combine_with_next(editor);
		return;
	}

	editor->dirty = true;

	current_line->used--;
	for ( size_t i = editor->cursor_column; i < current_line->used; i++ )
		current_line->data[i] = current_line->data[i+1];
}

void editor_type_delete_selection(struct editor* editor)
{
	if ( is_row_column_lt(editor->select_row, editor->select_column,
	                      editor->cursor_row, editor->cursor_column) )
	{
		size_t tmp;
		tmp = editor->select_row;
		editor->select_row = editor->cursor_row;
		editor->cursor_row = tmp;
		tmp = editor->select_column;
		editor->select_column = editor->cursor_column;
		editor->cursor_column = tmp;
	}

	size_t desired_row = editor->cursor_row;
	size_t desired_column = editor->cursor_column;

	editor->cursor_row = editor->select_row;
	editor->cursor_column = editor->select_column;

	while ( !(editor->cursor_row == desired_row &&
	          editor->cursor_column == desired_column) )
		editor_type_backspace(editor);
}

void editor_type_left(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
		return;
	}
	if ( editor->cursor_column )
		editor_cursor_column_dec(editor);
	else if ( editor->cursor_row )
	{
		editor_cursor_row_dec(editor);
		editor_cursor_column_set(editor, editor->lines[editor->cursor_row].used);
	}
}

void editor_type_select_left(struct editor* editor)
{
	if ( editor->select_column )
		editor_select_column_dec(editor);
	else if ( editor->select_row )
	{
		editor_select_row_dec(editor);
		editor_select_column_set(editor, editor->lines[editor->select_row].used);
	}
}

void editor_type_right(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                  editor->select_row, editor->select_column,
		                  &column, &row);
		editor_cursor_set(editor, column, row);
		return;
	}
	struct line* current_line = &editor->lines[editor->cursor_row];
	if ( editor->cursor_column != current_line->used )
		editor_cursor_column_inc(editor);
	else if ( editor->cursor_row+1 != editor->lines_used )
		editor_cursor_row_inc(editor),
		editor_cursor_column_set(editor, 0);
}

void editor_type_select_right(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	if ( editor->select_column != current_line->used )
		editor_select_column_inc(editor);
	else if ( editor->select_row+1 != editor->lines_used )
		editor_select_row_inc(editor),
		editor_select_column_set(editor, 0);
}

void editor_type_up(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( !editor->cursor_row )
	{
		editor_cursor_column_set(editor, 0);
		return;
	}
	size_t new_line_len = editor->lines[editor_cursor_row_dec(editor)].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_up(struct editor* editor)
{
	if ( !editor->select_row )
	{
		editor_select_column_set(editor, 0);
		return;
	}
	size_t new_line_len = editor->lines[editor_select_row_dec(editor)].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_type_down(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                   editor->select_row, editor->select_column,
		                   &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( editor->cursor_row+1 == editor->lines_used  )
	{
		editor_cursor_column_set(editor, editor->lines[editor->cursor_row].used);
		return;
	}
	size_t new_line_len = editor->lines[editor_cursor_row_inc(editor)].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_down(struct editor* editor)
{
	if ( editor->select_row+1 == editor->lines_used  )
	{
		editor_select_column_set(editor, editor->lines[editor->select_row].used);
		return;
	}
	size_t new_line_len = editor->lines[editor_select_row_inc(editor)].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_skip_leading(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->cursor_row];
	for ( editor_cursor_column_set(editor, 0);
	      editor->cursor_column < current_line->used;
	      editor_cursor_column_inc(editor) )
		if ( !isspace(current_line->data[editor->cursor_column]) )
			break;
}

void editor_select_skip_leading(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	for ( editor_select_column_set(editor, 0);
	      editor->select_column < current_line->used;
	      editor_select_column_inc(editor) )
		if ( !isspace(current_line->data[editor->select_column]) )
			break;
}

void editor_type_home(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( !editor->cursor_column )
	{
		editor_skip_leading(editor);
		return;
	}
	editor_cursor_column_set(editor, 0);
}

void editor_type_select_home(struct editor* editor)
{
	if ( !editor->select_column )
	{
		editor_select_skip_leading(editor);
		return;
	}
	editor_select_column_set(editor, 0);
}

void editor_skip_ending(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->cursor_row];
	for ( editor_cursor_column_set(editor, current_line->used);
	      editor->cursor_column;
	      editor_cursor_column_dec(editor) )
		if ( !isspace(current_line->data[editor->cursor_column-1]) )
			break;
}

void editor_select_skip_ending(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	for ( editor_select_column_set(editor, current_line->used);
	      editor->select_column;
	      editor_select_column_dec(editor) )
		if ( !isspace(current_line->data[editor->select_column-1]) )
			break;
}

void editor_type_end(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                   editor->select_row, editor->select_column,
		                   &column, &row);
		editor_cursor_set(editor, column, row);
	}
	struct line* current_line = &editor->lines[editor->cursor_row];
	if ( editor->cursor_column == current_line->used )
	{
		editor_skip_ending(editor);
		return;
	}
	editor_cursor_column_set(editor, current_line->used);
}

void editor_type_select_end(struct editor* editor)
{
	struct line* current_line = &editor->lines[editor->select_row];
	if ( editor->select_column == current_line->used )
	{
		editor_select_skip_ending(editor);
		return;
	}
	editor_select_column_set(editor, current_line->used);
}

void editor_type_page_up(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_smallest(editor->cursor_row, editor->cursor_column,
		                    editor->select_row, editor->select_column,
		                    &column, &row);
		editor_cursor_set(editor, column, row);
	}
	if ( editor->cursor_row < editor->viewport_height )
	{
		editor_cursor_set(editor, 0, 0);
		return;
	}
	size_t new_line = editor->cursor_row - editor->viewport_height;
	editor_cursor_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_page_up(struct editor* editor)
{
	if ( editor->select_row < editor->viewport_height )
	{
		editor_select_set(editor, 0, 0);
		return;
	}
	size_t new_line = editor->select_row - editor->viewport_height;
	editor_select_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_type_page_down(struct editor* editor)
{
	if ( editor_has_selection(editor) )
	{
		size_t column, row;
		row_column_biggest(editor->cursor_row, editor->cursor_column,
		                   editor->select_row, editor->select_column,
		                   &column, &row);
		editor_cursor_set(editor, column, row);
	}
	size_t new_line = editor->cursor_row + editor->viewport_height;
	if ( editor->lines_used <= new_line )
	{
		editor_cursor_row_set(editor, editor->lines_used - 1);
		editor_cursor_column_set(editor, editor->lines[editor->cursor_row].used);
		return;
	}
	editor_cursor_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->cursor_column )
		editor_cursor_column_set(editor, new_line_len);
}

void editor_type_select_page_down(struct editor* editor)
{
	size_t new_line = editor->select_row + editor->viewport_height;
	if ( editor->lines_used <= new_line )
	{
		editor_select_row_set(editor, editor->lines_used - 1);
		editor_select_column_set(editor, editor->lines[editor->select_row].used);
		return;
	}
	editor_select_row_set(editor, new_line);
	size_t new_line_len = editor->lines[new_line].used;
	if ( new_line_len < editor->select_column )
		editor_select_column_set(editor, new_line_len);
}

void editor_type_edit(struct editor* editor)
{
	editor->mode = MODE_EDIT;
}

void editor_type_goto_line(struct editor* editor)
{
	editor->mode = MODE_GOTO_LINE;
	editor->modal_used = 0;
	editor->modal_cursor = 0;
	editor->modal_error = false;
}

void editor_type_save(struct editor* editor)
{
	editor->mode = MODE_SAVE;

	free(editor->modal);
	editor->modal = strdup_safe(editor->current_file_name);
	editor->modal_used = strlen(editor->modal);
	editor->modal_length = editor->modal_used+1;
	editor->modal_cursor = editor->modal_used;
	editor->modal_error = false;
}

void editor_type_save_as(struct editor* editor)
{
	editor->mode = MODE_SAVE;
	editor->modal_used = 0;
	editor->modal_cursor = 0;
	editor->modal_error = false;
}

void editor_type_open(struct editor* editor)
{
	editor->mode = MODE_LOAD;
	editor->modal_used = 0;
	editor->modal_cursor = 0;
	editor->modal_error = false;
}

void editor_type_open_as(struct editor* editor)
{
	editor->mode = MODE_LOAD;

	free(editor->modal);
	editor->modal = strdup_safe(editor->current_file_name);
	editor->modal_used = strlen(editor->modal);
	editor->modal_length = editor->modal_used+1;
	editor->modal_cursor = editor->modal_used;
	editor->modal_error = false;
}

void editor_type_quit(struct editor* editor)
{
	editor->mode = editor->dirty ? MODE_ASK_QUIT : MODE_QUIT;
	editor->modal_cursor = 0;
	editor->modal_used = 0;
	editor->modal_error = false;
}

void editor_type_command(struct editor* editor)
{
	editor->mode = MODE_COMMAND;
	editor->modal_cursor = 0;
	editor->modal_used = 0;
	editor->modal_error = false;
}

void editor_type_raw_character(struct editor* editor, char c)
{
	struct line* current_line = &editor->lines[editor->cursor_row];

	if ( current_line->used == current_line->length )
	{
		size_t new_length = current_line->length ? 2 * current_line->length : 8;
		char* new_data = new char[new_length];
		for ( size_t i = 0; i < current_line->used; i++ )
			new_data[i] = current_line->data[i];
		delete[] current_line->data;
		current_line->data = new_data;
		current_line->length = new_length;
	}

	editor->dirty = true;

	for ( size_t i = current_line->used; editor->cursor_column < i; i-- )
		current_line->data[i] = current_line->data[i-1];
	current_line->used++;
	current_line->data[editor_cursor_column_inc(editor)-1] = c;
}

void editor_type_copy(struct editor* editor)
{
	if ( editor->cursor_row == editor->select_row &&
	     editor->cursor_column == editor->select_column )
		return;

	delete[] editor->clipboard;

	size_t start_row;
	size_t start_column;
	size_t end_row;
	size_t end_column;
	if ( is_row_column_lt(editor->select_row, editor->select_column,
	                      editor->cursor_row, editor->cursor_column) )
	{
		start_row = editor->select_row;
		start_column = editor->select_column;
		end_row = editor->cursor_row;
		end_column = editor->cursor_column;
	}
	else
	{
		start_row = editor->cursor_row;
		start_column = editor->cursor_column;
		end_row = editor->select_row;
		end_column = editor->select_column;
	}

	size_t length = 0;
	for ( size_t row = start_row, column = start_column;
	      is_row_column_lt(row, column, end_row, end_column); )
	{
		if ( row == end_row )
		{
			length += end_column - column;
			column = end_column;
		}
		else
		{
			length += editor->lines[row].used + 1 /*newline*/;
			column = 0;
			row++;
		}
	}

	editor->clipboard = new char[length + 1];
	size_t offset = 0;
	for ( size_t row = start_row, column = start_column;
	      is_row_column_lt(row, column, end_row, end_column); )
	{
		struct line* line = &editor->lines[row];
		if ( row == end_row )
		{
			memcpy(editor->clipboard + offset, line->data + column, end_column - column);
			offset += end_column - column;
			column = end_column;
		}
		else
		{
			memcpy(editor->clipboard + offset, line->data, line->used);
			editor->clipboard[offset + line->used] = '\n';
			offset += line->used + 1 /*newline*/;
			column = 0;
			row++;
		}
	}
	editor->clipboard[length] = '\0';
}

void editor_type_cut(struct editor* editor)
{
	if ( editor->cursor_row == editor->select_row &&
	     editor->cursor_column == editor->select_column )
		return;

	editor_type_copy(editor);
	editor_type_delete_selection(editor);
}

void editor_type_paste(struct editor* editor)
{
	if ( editor->cursor_row == editor->select_row &&
	     editor->cursor_column == editor->select_column )
		editor_type_delete_selection(editor);

	for ( size_t i = 0; editor->clipboard && editor->clipboard[i]; i++ )
	{
		if ( editor->clipboard[i] == '\n' )
			editor_type_newline(editor);
		else
			editor_type_raw_character(editor, editor->clipboard[i]);
	}
}

void editor_type_character(struct editor* editor, char c)
{
	if ( editor->control )
	{
		switch ( tolower(c) )
		{
		case 'c': editor_type_copy(editor); break;
		case 'i': editor_type_goto_line(editor); break;
		case 'k': editor_type_cut(editor); break;
		case 'o': editor->shift ?
		          editor_type_open_as(editor) :
		          editor_type_open(editor); break;
		case 'q': editor_type_quit(editor); break;
		case 's': editor->shift ?
		          editor_type_save_as(editor) :
		          editor_type_save(editor); break;
		case 'v': editor_type_paste(editor); break;
		case 'x': editor_type_cut(editor); break;
		}
		return;
	}

	if ( editor_has_selection(editor) )
		editor_type_delete_selection(editor);

	if ( c == '\n' ) { editor_type_newline(editor); return; }

	editor_type_raw_character(editor, c);
}

void editor_type_kbkey(struct editor* editor, int kbkey)
{
	if ( kbkey < 0 )
		return;

	if ( kbkey == KBKEY_ESC )
	{
		editor_type_command(editor);
		return;
	}

	if ( editor->control && editor->shift )
	{
		switch ( kbkey )
		{
		}
	}
	else if ( editor->control && !editor->shift )
	{
		switch ( kbkey )
		{
		}
	}
	else if ( !editor->control && editor->shift )
	{
		switch ( kbkey )
		{
		case KBKEY_LEFT: editor_type_select_left(editor); break;
		case KBKEY_RIGHT: editor_type_select_right(editor); break;
		case KBKEY_UP: editor_type_select_up(editor); break;
		case KBKEY_DOWN: editor_type_select_down(editor); break;
		case KBKEY_HOME: editor_type_select_home(editor); break;
		case KBKEY_END: editor_type_select_end(editor); break;
		case KBKEY_PGUP: editor_type_select_page_up(editor); break;
		case KBKEY_PGDOWN: editor_type_select_page_down(editor); break;
		case KBKEY_BKSPC: editor_type_backspace(editor); break;
		case KBKEY_DELETE: editor_type_delete(editor); break;
		}
	}
	else if ( !editor->control && !editor->shift )
	{
		switch ( kbkey )
		{
		case KBKEY_LEFT: editor_type_left(editor); break;
		case KBKEY_RIGHT: editor_type_right(editor); break;
		case KBKEY_UP: editor_type_up(editor); break;
		case KBKEY_DOWN: editor_type_down(editor); break;
		case KBKEY_HOME: editor_type_home(editor); break;
		case KBKEY_END: editor_type_end(editor); break;
		case KBKEY_PGUP: editor_type_page_up(editor); break;
		case KBKEY_PGDOWN: editor_type_page_down(editor); break;
		case KBKEY_BKSPC: editor_type_backspace(editor); break;
		case KBKEY_DELETE: editor_type_delete(editor); break;
		}
	}
}

void editor_modal_left(struct editor* editor)
{
	if ( editor->modal_cursor )
		editor->modal_cursor--;
}

void editor_modal_right(struct editor* editor)
{
	if ( editor->modal_cursor != editor->modal_used )
		editor->modal_cursor++;
}

void editor_modal_home(struct editor* editor)
{
	editor->modal_cursor = 0;
}

void editor_modal_end(struct editor* editor)
{
	editor->modal_cursor = editor->modal_used;
}

void editor_modal_backspace(struct editor* editor)
{
	if ( !editor->modal_cursor )
		return;

	editor->modal_error = false;

	editor->modal_used--;
	for ( size_t i = --editor->modal_cursor; i < editor->modal_used; i++ )
		editor->modal[i] = editor->modal[i+1];
}

void editor_modal_delete(struct editor* editor)
{
	if ( editor->modal_cursor == editor->modal_used )
		return;

	editor->modal_error = false;

	editor->modal_used--;
	for ( size_t i = editor->modal_cursor; i < editor->modal_used; i++ )
		editor->modal[i] = editor->modal[i+1];
}

void editor_reset_contents(struct editor* editor)
{
	for ( size_t i = 0; i < editor->lines_used; i++ )
		delete[] editor->lines[i].data;
	delete[] editor->lines;

	editor->lines_used = 1;
	editor->lines_length = 1;
	editor->lines = new struct line[editor->lines_length];
	editor->lines[0].data = NULL;
	editor->lines[0].used = 0;
	editor->lines[0].length = 0;
	editor->highlight_source = false;
	editor_cursor_set(editor, 0, 0);
}

bool editor_load_file_contents(struct editor* editor, FILE* fp)
{
	struct stat st;
	if ( fstat(fileno(fp), &st) != 0 || S_ISDIR(st.st_mode) )
		return errno = EISDIR, false;

	free(editor->current_file_name);
	editor->current_file_name = NULL;

	editor_reset_contents(editor);

	bool last_newline = false;
	int ic;
	while ( (ic = fgetc(fp)) != EOF )
	{
		if ( last_newline )
			editor_type_newline(editor);
		if ( ic < 128 && !(last_newline = ic == '\n') )
			editor_type_raw_character(editor, (char) ic);
	}

	editor_cursor_set(editor, 0, 0);

	return true;
}

bool should_highlight_path(const char* path)
{
	size_t path_length = strlen(path);
	if ( 2 <= path_length &&
	     (!strcmp(path+path_length-2, ".c") ||
		  !strcmp(path+path_length-2, ".h")) )
		return true;
	if ( 4 <= path_length &&
	     (!strcmp(path+path_length-4, ".c++") ||
		  !strcmp(path+path_length-4, ".h++") ||
		  !strcmp(path+path_length-4, ".cxx") ||
		  !strcmp(path+path_length-4, ".hxx") ||
		  !strcmp(path+path_length-4, ".cpp") ||
		  !strcmp(path+path_length-4, ".hpp")) )
		return true;
	return false;
}

bool editor_load_file(struct editor* editor, const char* path)
{
	if ( FILE* fp = fopen(path, "r") )
	{
		bool success = editor_load_file_contents(editor, fp);
		fclose(fp);
		if ( !success )
			return false;
		editor->dirty = false;
	}
	else if ( errno == ENOENT )
	{
		editor_reset_contents(editor);
		editor->dirty = true;
	}
	else
		return false;

	editor->current_file_name = strdup(path);
	editor->highlight_source = should_highlight_path(path);

	return true;
}

bool editor_load_popen(struct editor* editor, const char* cmd)
{
	FILE* fp = popen(cmd, "r");
	if ( !fp )
		return false;
	bool success = editor_load_file_contents(editor, fp);
	pclose(fp);

	if ( !success )
		return false;

	editor->current_file_name = NULL;
	editor->dirty = true;

	return true;
}

bool editor_save_file(struct editor* editor, const char* path)
{
	FILE* fp = fopen(path, "w");
	if ( !fp )
		return false;

	for ( size_t i = 0; i < editor->lines_used; i++ )
	{
		fwrite(editor->lines[i].data, sizeof(char), editor->lines[i].used, fp);
		fputc('\n', fp);
	}

	editor->current_file_name = strdup(path);
	editor->dirty = false;
	editor->highlight_source = should_highlight_path(path);

	return fclose(fp) != EOF;
}

void editor_modal_load(struct editor* editor, const char* path)
{
	if ( editor_load_file(editor, path) )
		editor_type_edit(editor);
	else
		editor->modal_error = true;
}

void editor_modal_save(struct editor* editor, const char* path)
{
	if ( editor_save_file(editor, path) )
		editor_type_edit(editor);
	else
		editor->modal_error = true;
}

void editor_modal_ask_quit(struct editor* editor, const char* answer)
{
	if ( tolower(answer[0]) == 'y' )
		editor->mode = MODE_QUIT;
	else if ( tolower(answer[0]) == 'n' || !answer[0] )
		editor_type_edit(editor);
	else
		editor->modal_error = true;
}

void editor_modal_goto_line(struct editor* editor, const char* linestr)
{
	if ( linestr[0] )
	{
		bool go_back = false, go_forward = false;
		if ( linestr[0] == '+' )
			linestr++, go_forward = true;
		else if ( linestr[0] == '-' )
			linestr++, go_back = true;
		if ( !linestr[0] ) { editor->modal_error = true; return; }
		const char* linestr_end;
		unsigned long line = strtoul(linestr, (char**) &linestr_end, 0);
		if ( *linestr_end ) { editor->modal_error = true; return; }
		if ( go_back )
		{
			if ( editor->cursor_row < line )
			{
				editor->modal_error = true;
				return;
			}
			editor_cursor_row_set(editor, editor->cursor_row - line);
		}
		else if ( go_forward )
		{
			if ( editor->lines_used - (editor->cursor_row+1) < line )
			{
				editor->modal_error = true;
				return;
			}
			editor_cursor_row_set(editor, editor->cursor_row + line);
		}
		else
		{
			if ( editor->lines_used+1 <= line )
			{
				editor->modal_error = true;
				return;
			}
			editor_cursor_row_set(editor, line ? line - 1 : 0);
		}
		editor_cursor_column_set(editor, 0);
	}
	editor_type_edit(editor);
}

void editor_modal_margin(struct editor* editor, const char* marginstr)
{
	if ( !marginstr[0] )
		editor->margin = SIZE_MAX;
	else
	{
		char* end_ptr;
		unsigned long margin = strtoul(marginstr, &end_ptr, 0);
		if ( *end_ptr ) { editor->modal_error = true; return; }
		editor->margin = margin;
	}
	editor_type_edit(editor);
}

void editor_modal_popen(struct editor* editor, const char* cmd)
{
	if ( cmd[0] && editor_load_popen(editor, cmd) )
		editor_type_edit(editor);
	else
		editor->modal_error = true;
}

void editor_modal_tabsize(struct editor* editor, const char* tabsizestr)
{
	if ( !tabsizestr[0] )
		editor->tabsize = 8;
	else
	{
		char* end_ptr;
		unsigned long tabsize = strtoul(tabsizestr, &end_ptr, 0);
		if ( !tabsize || *end_ptr || 256 < tabsize )
		{
			editor->modal_error = true;
			return;
		}
		editor->tabsize = tabsize;
	}
	editor_type_edit(editor);
}

void editor_modal_language(struct editor* editor, const char* language)
{
	if ( !language[0] || !strcmp(language, "none") )
	{
		editor->highlight_source = false;
		return;
	}
	if ( !strcmp(language, "c") || !strcmp(language, "c++") )
	{
		editor->highlight_source = true;
		return;
	}
	editor->modal_error = true;
	editor_type_edit(editor);
}

bool is_modal_command(const char* cmd, const char* candidate, const char** rest)
{
	size_t candidate_len = strlen(candidate);
	if ( strncmp(cmd, candidate, candidate_len) == 0 &&
	     (!cmd[candidate_len] || isspace(cmd[candidate_len])) )
	{
		*rest = cmd + candidate_len;
		while ( **rest && isspace(**rest) )
			(*rest)++;
		return true;
	}
	return false;
}

void editor_modal_command(struct editor* editor, const char* cmd)
{
	while ( *cmd && isspace(*cmd) )
		cmd++;
	if ( cmd[0] == ':' )
		cmd++;
	if ( !cmd[0] ) { editor_type_edit(editor); return; }

	if ( !strcmp(cmd, "q") || !strcmp(cmd, "exit") || !strcmp(cmd, "quit") )
		editor_type_quit(editor);
	else if ( !strcmp(cmd, "q!") )
		editor->dirty = false, editor_type_quit(editor);
	else if ( !strcmp(cmd, "w") )
		editor_type_save(editor);
	else if ( !strcmp(cmd, "wq") || !strcmp(cmd, "wq!") )
		editor->dirty ? editor_type_save(editor)
		              : editor_type_quit(editor);
	else if ( is_modal_command(cmd, "margin", &cmd) )
		editor_modal_margin(editor, cmd);
	else if ( is_modal_command(cmd, "popen", &cmd) )
		editor_modal_popen(editor, cmd);
	else if ( is_modal_command(cmd, "tabsize", &cmd) )
		editor_modal_tabsize(editor, cmd);
	else if ( is_modal_command(cmd, "language", &cmd) )
		editor_modal_language(editor, cmd);
	else
		editor->modal_error = true;
}

void editor_modal_character(struct editor* editor, char c)
{
	if ( editor->control )
	{
		switch ( tolower(c) )
		{
		case 'c': editor_type_edit(editor); break;
		}
		return;
	}

	editor->modal_error = false;

	if ( c == '\n' )
	{
		char* param = new char[editor->modal_used+1];
		memcpy(param, editor->modal, editor->modal_used);
		param[editor->modal_used] = '\0';
		switch ( editor->mode )
		{
		case MODE_LOAD: editor_modal_load(editor, param); break;
		case MODE_SAVE: editor_modal_save(editor, param); break;
		case MODE_ASK_QUIT: editor_modal_ask_quit(editor, param); break;
		case MODE_GOTO_LINE: editor_modal_goto_line(editor, param); break;
		case MODE_COMMAND: editor_modal_command(editor, param); break;
		default: break;
		}
		free(param);
		return;
	}

	if ( editor->modal_used == editor->modal_length )
	{
		size_t new_length = editor->modal_length ? 2 * editor->modal_length : 8;
		char* new_data = new char[new_length];
		for ( size_t i = 0; i < editor->modal_used; i++ )
			new_data[i] = editor->modal[i];
		delete[] editor->modal;
		editor->modal = new_data;
		editor->modal_length = new_length;
	}

	for ( size_t i = editor->modal_used; editor->modal_cursor < i; i-- )
		editor->modal[i] = editor->modal[i-1];
	editor->modal_used++;
	editor->modal[editor->modal_cursor++] = c;
}

void editor_modal_kbkey(struct editor* editor, int kbkey)
{
	if ( editor->control )
		return;

	if ( kbkey < 0 )
		return;

	switch ( kbkey )
	{
	case KBKEY_LEFT: editor_modal_left(editor); break;
	case KBKEY_RIGHT: editor_modal_right(editor); break;
	case KBKEY_HOME: editor_modal_home(editor); break;
	case KBKEY_END: editor_modal_end(editor); break;
	case KBKEY_BKSPC: editor_modal_backspace(editor); break;
	case KBKEY_DELETE: editor_modal_delete(editor); break;
	case KBKEY_ESC: editor_type_edit(editor); break;
	}
}

void editor_codepoint(struct editor* editor, uint32_t codepoint)
{
	if ( 128 <= codepoint )
		return;

	char c = (char) codepoint;

	if ( c == '\b' || c == 127 )
		return;

	if ( editor->mode == MODE_EDIT )
		editor_type_character(editor, c);
	else
		editor_modal_character(editor, c);
}

void editor_kbkey(struct editor* editor, int kbkey)
{
	int abskbkey = kbkey < 0 ? -kbkey : kbkey;

	if ( abskbkey == KBKEY_LCTRL )
	{
		editor->control = 0 <= kbkey;
		return;
	}
	if ( abskbkey == KBKEY_LSHIFT )
	{
		editor->lshift = 0 <= kbkey;
		editor->shift = editor->lshift || editor->rshift;
		return;
	}
	if ( abskbkey == KBKEY_RSHIFT )
	{
		editor->rshift = 0 <= kbkey;
		editor->shift = editor->lshift || editor->rshift;
		return;
	}

	if ( editor->mode == MODE_EDIT )
		editor_type_kbkey(editor, kbkey);
	else
		editor_modal_kbkey(editor, kbkey);
}

int main(int argc, char* argv[])
{
	if ( !isatty(0) )
		error(1, errno, "standard input");
	if ( !isatty(1) )
		error(1, errno, "standard output");

	struct editor editor;
	initialize_editor(&editor);

	if ( 2 <= argc && !editor_load_file(&editor, argv[1]) )
		error(1, errno, "`%s'", argv[1]);

	unsigned old_termmode;
	gettermmode(0, &old_termmode);
	settermmode(0, TERMMODE_KBKEY | TERMMODE_UNICODE);

	struct terminal_state stdout_state;
	make_terminal_state(stdout, &stdout_state);
	reset_terminal_state(stdout, &stdout_state);
	fflush(stdout);

	while ( editor.mode != MODE_QUIT )
	{
		struct terminal_state output_state;
		make_terminal_state(stdout, &output_state);
		editor_colorize(&editor);
		render_editor(&editor, &output_state);
		update_terminal(stdout, &output_state, &stdout_state);
		free_terminal_state(&output_state);
		fflush(stdout);

		uint32_t input;
		if ( read(0, &input, sizeof(input)) != sizeof(input) )
			break;
		if ( int kbkey = KBKEY_DECODE(input) )
			editor_kbkey(&editor, kbkey);
		else
			editor_codepoint(&editor, input);
	}

	reset_terminal_state(stdout, &stdout_state);
	free_terminal_state(&stdout_state);
	fflush(stdout);

	settermmode(0, old_termmode);

	return 0;
}

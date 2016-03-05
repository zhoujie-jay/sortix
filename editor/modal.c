/*
 * Copyright (c) 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * modal.c
 * Modal commands.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "command.h"
#include "cursor.h"
#include "editor.h"
#include "highlight.h"
#include "modal.h"
#include "multibyte.h"

static void editor_reset_modal(struct editor* editor)
{
	editor->modal_used = 0;
	editor->modal_cursor = 0;
}

bool is_truth_string(const char* truth)
{
	return !strcmp(truth, "on") || !strcmp(truth, "off");
}

bool is_truth_true(const char* truth)
{
	return strcmp(truth, "off") != 0;
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

void editor_modal_save_load(struct editor* editor, const char* path)
{
	if ( editor_save_file(editor, path) )
	{
		editor_reset_modal(editor);
		editor->mode = MODE_LOAD;
	}
	else
		editor->modal_error = true;
}

void editor_modal_save_quit(struct editor* editor, const char* path)
{
	if ( editor_save_file(editor, path) )
		editor->mode = MODE_QUIT;
	else
		editor->modal_error = true;
}

void editor_modal_ask_load(struct editor* editor, const char* answer)
{
	if ( tolower((unsigned char) answer[0]) == 'y' )
	{
		editor_reset_modal(editor);
		if ( editor->current_file_name )
		{
			if ( editor_save_file(editor, editor->current_file_name) )
			{
				editor->mode = MODE_LOAD;
				return;
			}
			editor->modal_error = true;
		}
		editor->mode = MODE_SAVE_LOAD;
	}
	else if ( tolower((unsigned char) answer[0]) == 'n' )
	{
		editor_reset_modal(editor);
		editor->mode = MODE_LOAD;
	}
	else if ( !answer[0] )
		editor_type_edit(editor);
	else
		editor->modal_error = true;
}

void editor_modal_ask_quit(struct editor* editor, const char* answer)
{
	if ( tolower((unsigned char) answer[0]) == 'y' )
	{
		editor_reset_modal(editor);
		if ( editor->current_file_name )
		{
			if ( editor_save_file(editor, editor->current_file_name) )
			{
				editor->mode = MODE_QUIT;
				return;
			}
			editor->modal_error = true;
		}
		editor->mode = MODE_SAVE_QUIT;
	}
	else if ( tolower((unsigned char) answer[0]) == 'n' )
		editor->mode = MODE_QUIT;
	else if ( !answer[0] )
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
		editor->highlight_source = LANGUAGE_NONE;
	else if ( !strcmp(language, "c") || !strcmp(language, "c++") )
		editor->highlight_source = LANGUAGE_C_CXX;
	else if ( !strcmp(language, "diff") || !strcmp(language, "patch") )
		editor->highlight_source = LANGUAGE_DIFF;
	else
	{
		editor->modal_error = true;
		return;
	}
	editor_type_edit(editor);
}

void editor_modal_line_numbering(struct editor* editor, const char* truth)
{
	if ( !is_truth_string(truth) )
	{
		editor->modal_error = true;
		return;
	}
	editor->line_numbering = is_truth_true(truth);
	editor_type_edit(editor);
}

bool is_modal_command(const char* cmd, const char* candidate, const char** rest)
{
	size_t candidate_len = strlen(candidate);
	if ( strncmp(cmd, candidate, candidate_len) == 0 &&
	     (!cmd[candidate_len] || isspace((unsigned char) cmd[candidate_len])) )
	{
		*rest = cmd + candidate_len;
		while ( **rest && isspace((unsigned char) **rest) )
			(*rest)++;
		return true;
	}
	return false;
}

void editor_modal_command(struct editor* editor, const char* cmd)
{
	while ( *cmd && isspace((unsigned char) *cmd) )
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
	else if ( is_modal_command(cmd, "line-numbering", &cmd) )
		editor_modal_line_numbering(editor, cmd);
	else
		editor->modal_error = true;
}

void editor_modal_command_config(struct editor* editor, const char* cmd)
{
	while ( *cmd && isspace((unsigned char) *cmd) )
		cmd++;
	if ( is_modal_command(cmd, "margin", &cmd) )
		editor_modal_margin(editor, cmd);
	else if ( is_modal_command(cmd, "tabsize", &cmd) )
		editor_modal_tabsize(editor, cmd);
	else if ( is_modal_command(cmd, "language", &cmd) )
		editor_modal_language(editor, cmd);
	else if ( is_modal_command(cmd, "line-numbering", &cmd) )
		editor_modal_line_numbering(editor, cmd);
}


void editor_modal_character(struct editor* editor, wchar_t c)
{
	if ( editor->control )
	{
		switch ( towlower(c) )
		{
		case L'c': editor_type_edit(editor); break;
		}
		return;
	}

	editor->modal_error = false;

	if ( c == L'\n' )
	{
		if ( !editor->modal )
			editor->modal = (wchar_t*) malloc(sizeof(wchar_t) * 1);

		editor->modal[editor->modal_used] = L'\0';
		char* param = convert_wcs_to_mbs(editor->modal);
		switch ( editor->mode )
		{
		case MODE_LOAD: editor_modal_load(editor, param); break;
		case MODE_SAVE: editor_modal_save(editor, param); break;
		case MODE_SAVE_LOAD: editor_modal_save_load(editor, param); break;
		case MODE_SAVE_QUIT: editor_modal_save_quit(editor, param); break;
		case MODE_ASK_LOAD: editor_modal_ask_load(editor, param); break;
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
		wchar_t* new_data = (wchar_t*) malloc(sizeof(wchar_t) * (new_length + 1));
		for ( size_t i = 0; i < editor->modal_used; i++ )
			new_data[i] = editor->modal[i];
		free(editor->modal);
		editor->modal = new_data;
		editor->modal_length = new_length;
	}

	for ( size_t i = editor->modal_used; editor->modal_cursor < i; i-- )
		editor->modal[i] = editor->modal[i-1];
	editor->modal_used++;
	editor->modal[editor->modal_cursor++] = c;
}

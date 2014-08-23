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

    modal.c++
    Modal commands.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#include "command.h++"
#include "cursor.h++"
#include "editor.h++"
#include "modal.h++"
#include "multibyte.h++"

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

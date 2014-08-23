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

    editor.c++
    Editor.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>

#include "command.h++"
#include "cursor.h++"
#include "display.h++"
#include "editor.h++"
#include "highlight.h++"
#include "input.h++"
#include "terminal.h++"

void initialize_editor(struct editor* editor)
{
	memset(editor, 0, sizeof(*editor));

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

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	bool last_newline = false;
	int ic;
	while ( (ic = fgetc(fp)) != EOF )
	{
		if ( last_newline )
		{
			editor_type_newline(editor);
			last_newline = false;
		}

		char c = (char) ic;
		wchar_t wc;
		size_t count = mbrtowc(&wc, &c, 1, &ps);
		if ( count == (size_t) 0 )
			continue;
		if ( count == (size_t) -1 )
		{
			memset(&ps, 0, sizeof(ps));
			wc = L'�';
		}
		if ( count == (size_t) -2 )
			continue;
		assert(wc != L'\0');
		if ( !(last_newline = wc == L'\n') )
			editor_type_raw_character(editor, wc);
	}

	if ( !mbsinit(&ps) )
		editor_type_raw_character(editor, L'�');

	editor_cursor_set(editor, 0, 0);

	return true;
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

	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	for ( size_t i = 0; i < editor->lines_used; i++ )
	{
		char mb[MB_CUR_MAX];
		for ( size_t n = 0; n < editor->lines[i].used; n++ )
		{
			mbstate_t saved_ps = ps;
			wchar_t wc = editor->lines[i].data[n];

			size_t count = wcrtomb(mb, wc, &ps);
			if ( count == (size_t) -1 )
			{
				ps = saved_ps;
				count = wcrtomb(mb, L'�', &ps);
				assert(count != (size_t) -1);
			}
			fwrite(mb, sizeof(char), count, fp);
		}
		size_t count = wcrtomb(mb, L'\n', &ps);
		assert(count != (size_t) -1);
		fwrite(mb, sizeof(char), count, fp);
	}

	editor->current_file_name = strdup(path);
	editor->dirty = false;
	editor->highlight_source = should_highlight_path(path);

	return fclose(fp) != EOF;
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	if ( !isatty(0) )
		error(1, errno, "standard input");
	if ( !isatty(1) )
		error(1, errno, "standard output");

	struct editor editor;
	initialize_editor(&editor);

	if ( 2 <= argc && !editor_load_file(&editor, argv[1]) )
		error(1, errno, "`%s'", argv[1]);

	struct editor_input editor_input;
	editor_input_begin(&editor_input);

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

		editor_input_process(&editor_input, &editor);
	}

	reset_terminal_state(stdout, &stdout_state);
	free_terminal_state(&stdout_state);
	fflush(stdout);

	editor_input_end(&editor_input);

	return 0;
}

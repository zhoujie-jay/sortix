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

    input.c++
    Keyboard input.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#if defined(__sortix__)
#include <sys/keycodes.h>
#include <sys/termmode.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "command.h++"
#include "editor.h++"
#include "input.h++"
#include "modal.h++"

void editor_codepoint(struct editor* editor, uint32_t codepoint)
{
	wchar_t c = (wchar_t) codepoint;

	if ( c == L'\b' || c == 127 /* delete */ )
		return;

	if ( editor->mode == MODE_EDIT )
		editor_type_character(editor, c);
	else
		editor_modal_character(editor, c);
}

#if defined(__sortix__)
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
		case KBKEY_LEFT: editor_type_control_select_left(editor); break;
		case KBKEY_RIGHT: editor_type_control_select_right(editor); break;
		case KBKEY_UP: editor_type_control_select_up(editor); break;
		case KBKEY_DOWN: editor_type_control_select_down(editor); break;
		}
	}
	else if ( editor->control && !editor->shift )
	{
		switch ( kbkey )
		{
		case KBKEY_LEFT: editor_type_control_left(editor); break;
		case KBKEY_RIGHT: editor_type_control_right(editor); break;
		case KBKEY_UP: editor_type_control_up(editor); break;
		case KBKEY_DOWN: editor_type_control_select_down(editor); break;
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
#endif

void editor_input_begin(struct editor_input* editor_input)
{
#if defined(__sortix__)
	gettermmode(0, &editor_input->saved_termmode);
	settermmode(0, TERMMODE_KBKEY | TERMMODE_UNICODE);
#else
	(void) editor_input;
#endif
}

void editor_input_process(struct editor_input* editor_input,
                          struct editor* editor)
{
#if defined(__sortix__)
	(void) editor_input;
	uint32_t input;
	if ( read(0, &input, sizeof(input)) != sizeof(input) )
		return;
	if ( int kbkey = KBKEY_DECODE(input) )
		editor_kbkey(editor, kbkey);
	else
		editor_codepoint(editor, input);
#else
	(void) editor_input;
	(void) editor;
#endif
}

void editor_input_end(struct editor_input* editor_input)
{
#if defined(__sortix__)
	settermmode(0, editor_input->saved_termmode);
#else
	(void) editor_input;
#endif
}

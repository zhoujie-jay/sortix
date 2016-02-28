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

    input.h
    Keyboard input.

*******************************************************************************/

#ifndef EDITOR_INPUT_H
#define EDITOR_INPUT_H

struct editor;

struct editor_input
{
#if defined(__sortix__)
	unsigned int saved_termmode;
#endif
};

void editor_input_begin(struct editor_input* editor_input);
void editor_input_process(struct editor_input* editor_input,
                          struct editor* editor);
void editor_input_end(struct editor_input* editor_input);

#endif

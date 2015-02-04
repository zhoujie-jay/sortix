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

    highlight.h++
    Syntax highlighting.

*******************************************************************************/

#ifndef EDITOR_HIGHLIGHT_HXX
#define EDITOR_HIGHLIGHT_HXX

struct editor;

bool should_highlight_path(const char* path);
void editor_colorize(struct editor* editor);

#endif
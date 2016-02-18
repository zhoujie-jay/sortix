/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

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

    interactive.h
    Interactive utility functions.

*******************************************************************************/

#ifndef INTERACTIVE_H
#define INTERACTIVE_H

extern const char* prompt_man_section;
extern const char* prompt_man_page;

void shlvl(void);
void text(const char* str);
__attribute__((format(printf, 1, 2)))
void textf(const char* format, ...);
void prompt(char* buffer,
            size_t buffer_size,
            const char* question,
            const char* answer);
void password(char* buffer,
              size_t buffer_size,
              const char* question);
bool missing_program(const char* program);

#endif

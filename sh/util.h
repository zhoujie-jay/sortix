/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    util.h
    Utility functions.

*******************************************************************************/

#ifndef UTIL_H
#define UTIL_H

char* strdup_safe(const char* string);
const char* getenv_safe(const char* name, const char* def = "");
bool array_add(void*** array_ptr,
               size_t* used_ptr,
               size_t* length_ptr,
               void* value);
bool might_need_shell_quote(char c);

struct stringbuf
{
	char* string;
	size_t length;
	size_t allocated_size;
};

void stringbuf_begin(struct stringbuf* buf);
void stringbuf_append_c(struct stringbuf* buf, char c);
char* stringbuf_finish(struct stringbuf* buf);

#endif

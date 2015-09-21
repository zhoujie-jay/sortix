/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    fstab/getfsent.cpp
    Read filesystem table entry.

*******************************************************************************/

#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" struct fstab* getfsent(void)
{
	if ( !__fstab_file && !setfsent() )
		return NULL;
	static struct fstab fs;
	static char* line = NULL;
	static size_t line_size = 0;
	ssize_t line_length;
	while ( 0 <= (line_length = getline(&line, &line_size, __fstab_file)) )
	{
		if ( line_length && line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		if ( scanfsent(line, &fs) )
			return &fs;
	}
	free(line);
	line = NULL;
	line_size = 0;
	return NULL;
}

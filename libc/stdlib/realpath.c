/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

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

    stdlib/realpath.c
    Return the canonicalized filename.

*******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <scram.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef PATH_MAX
#error "This realpath implementation assumes no PATH_MAX"
#endif

char* realpath(const char* restrict path, char* restrict resolved_path)
{
	if ( resolved_path )
	{
		struct scram_undefined_behavior info;
		info.filename = __FILE__;
		info.line = __LINE__;
		info.column = 0;
		info.violation = "realpath call with non-null argument and PATH_MAX unset";
		scram(SCRAM_UNDEFINED_BEHAVIOR, &info);
	}
	char* ret_path = canonicalize_file_name(path);
	if ( !ret_path )
		return NULL;
	return ret_path;
}

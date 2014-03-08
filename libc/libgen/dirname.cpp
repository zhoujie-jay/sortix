/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    libgen/dirname.cpp
    Returns the directory part of a path.

*******************************************************************************/

#include <libgen.h>
#include <string.h>

static const char current_directory[2] = ".";

extern "C" char* dirname(char* path)
{
	if ( !path || !*path )
		return (char*) current_directory;
	size_t path_len = strlen(path);
	while ( 2 <= path_len && path[path_len-1] == '/' )
		path[--path_len] = '\0';
	if ( strcmp(path, "/") == 0 )
		return path;
	if ( !strchr(path, '/') )
		return (char*) current_directory;
	while ( 1 <= path_len && path[path_len-1] != '/' )
		path[--path_len] = '\0';
	while ( 1 <= path_len && path[path_len-1] == '/' )
		path[--path_len] = '\0';
	if ( !path[0] )
		strcpy(path, "/");
	return path;
}

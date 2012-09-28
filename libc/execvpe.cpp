/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	execvpe.cpp
	Loads a process image.

*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Note that the only PATH variable in Sortix is the one used here.
extern "C" int execvpe(const char* filename, char* const* argv,
                       char* const* envp)
{
	if ( strchr(filename, '/') )
		return execve(filename, argv, envp);
	size_t filenamelen = strlen(filename);
	const char* PATH = "/bin";
	size_t pathlen = strlen(PATH);
	char* pathname = (char*) malloc(filenamelen + 1 + pathlen + 1);
	if ( !pathname ) { return -1; }
	stpcpy(stpcpy(stpcpy(pathname, PATH), "/"), filename);
	int result = execve(pathname, argv, envp);
	free(pathname);
	return result;
}

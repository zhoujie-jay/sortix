/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    unistd/execvpe.cpp
    Loads a process image.

*******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: Move this to some generic environment interface!
static const char* LookupEnvironment(const char* name, char* const* envp)
{
	size_t equalpos = strcspn(name, "=");
	if ( name[equalpos] == '=' )
		return NULL;
	size_t namelen = equalpos;
	for ( size_t i = 0; envp[i]; i++ )
	{
		if ( strncmp(name, envp[i], namelen) )
			continue;
		if ( envp[i][namelen] != '=' )
			continue;
		return envp[i] + namelen + 1;
	}
	return NULL;
}

// TODO: Provide an interface that allows user-space to find out which command
// would have been executed (according to PATH) had execvpe been called now.
// This is of value to programs such as which(1), instead of repeating much of
// this logic there.
extern "C" int execvpe(const char* filename, char* const* argv,
                       char* const* envp)
{
	const char* path = LookupEnvironment("PATH", envp);
	// TODO: Should there be a default PATH value?
	if ( strchr(filename, '/') || !path )
		return execve(filename, argv, envp);

	// Search each directory in the PATH variable for a suitable file.
	size_t filename_len = strlen(filename);
	while ( *path )
	{
		size_t len = strcspn(path, ":");
		if ( !len )
		{
			// Sortix doesn't support that the empty string means current
			// directory. While it does inductively make sense, the common
			// kernel interfaces such as openat doesn't accept it and software
			// often just prefix their directories and a colon to PATH without
			// regard to whether it's already empty. S
			path++;
			continue;
		}

		// Determine the full path to the file if it is in the directory.
		size_t fullpath_len = len + 1 + filename_len + 1;
		char* fullpath = (char*) malloc(fullpath_len * sizeof(char));
		if ( !fullpath )
			return -1;
		stpcpy(stpcpy(stpncpy(fullpath, path, len), "/"), filename);
		if ( (path += len + 1)[0] == ':' )
			path++;

		execve(fullpath, argv, envp);
		free(fullpath);

		// TODO: There may be some security concerns here as to whether to
		// continue or abort execution. For instance, if a directory in the
		// start of the PATH has permissions set up too restrictively, then
		// it would never look in the later directories (and you can't execute
		// anything without absolute paths). And other situations.
		if ( errno == EACCES )
			return -1;
		if ( errno == ENOENT )
			continue;

	}
	return -1;
}

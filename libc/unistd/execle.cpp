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

    unistd/execle.cpp
    Loads a program image.

*******************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" int execle(const char* pathname, const char* argv0, ...)
{
	if ( !argv0 )
	{
		va_list args;
		va_start(args, argv0);
		char* const* envp = va_arg(args, char* const*);
		va_end(args);
		return execve(pathname, (char* const*) &argv0, envp);
	}

	va_list args;
	va_start(args, argv0);
	size_t numargs = 1;
	while ( argv0 && va_arg(args, const char*) )
		numargs++;
	va_end(args);

	char** argv = (char**) malloc(sizeof(char*) * (numargs+1));
	if ( !argv )
		return -1;

	argv[0] = (char*) argv0;
	va_start(args, argv0);
	for ( size_t i = 1; i <= numargs; i++ )
		argv[i] = (char*) va_arg(args, const char*);
	char* const* envp = va_arg(args, char* const*);
	va_end(args);
	argv[numargs] = NULL;

	int result = execve(pathname, argv, envp);

	free(argv);

	return result;
}
